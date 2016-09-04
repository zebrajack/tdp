#include <pangolin/pangolin.h>
#include <pangolin/video/video_record_repeat.h>
#include <pangolin/gl/gltexturecache.h>
#include <pangolin/gl/glpixformat.h>
#include <pangolin/handler/handler_image.h>
#include <pangolin/utils/file_utils.h>
#include <pangolin/utils/timer.h>
#include <pangolin/gl/gl.h>
#include <pangolin/gl/glcuda.h>

#include <tdp/eigen/dense.h>
#include <tdp/managed_image.h>

#include <tdp/convolutionSeparable.h>
#include <tdp/depth.h>
#include <tdp/normals.h>
#include <tdp/camera.h>
#include <tdp/quickView.h>
#include <tdp/directional/hist.h>
#include <tdp/nvidia/helper_cuda.h>

#include "gui.hpp"

void VideoViewer(const std::string& input_uri, const std::string& output_uri)
{

  // Open Video by URI
  pangolin::VideoRecordRepeat video(input_uri, output_uri);
  const size_t num_streams = video.Streams().size();

  if(num_streams == 0) {
    pango_print_error("No video streams from device.\n");
    return;
  }

  GUI gui(1200,800,video);

  size_t w = video.Streams()[gui.iRGB].Width();
  size_t h = video.Streams()[gui.iRGB].Height();
  size_t wc = w+w%64; // for convolution
  size_t hc = h+h%64;
  float f = 550;
  float uc = (w-1.)/2.;
  float vc = (h-1.)/2.;

  tdp::QuickView viewDebugA(wc,hc);
  gui.container().AddDisplay(viewDebugA);
  tdp::QuickView viewDebugB(wc,hc);
  gui.container().AddDisplay(viewDebugB);
  tdp::QuickView viewN2D(wc,hc);
  gui.container().AddDisplay(viewN2D);

  // Define Camera Render Object (for view / scene browsing)
  pangolin::OpenGlRenderState s_cam(
      pangolin::ProjectionMatrix(640,480,420,420,320,240,0.1,1000),
      pangolin::ModelViewLookAt(0,0.5,-3, 0,0,0, pangolin::AxisY)
      );
  // Add named OpenGL viewport to window and provide 3D Handler
  pangolin::View& d_cam = pangolin::CreateDisplay()
    .SetHandler(new pangolin::Handler3D(s_cam));
  gui.container().AddDisplay(d_cam);

  tdp::ManagedHostImage<float> d(wc, hc);
  tdp::ManagedHostImage<float> debugA(wc, hc);
  tdp::ManagedHostImage<float> debugB(wc, hc);

  tdp::ManagedHostImage<Eigen::Matrix<uint8_t,3,1>> n2D(wc,hc);
  memset(n2D.ptr_,0,n2D.SizeBytes());
  tdp::ManagedHostImage<tdp::Vector3fda> n2Df(wc,hc);
  tdp::ManagedHostImage<tdp::Vector3fda> n(wc,hc);

  pangolin::GlBufferCudaPtr cuNbuf(pangolin::GlArrayBuffer, wc*hc,
      GL_FLOAT, 3, cudaGraphicsMapFlagsNone, GL_DYNAMIC_DRAW);

  tdp::ManagedDeviceImage<uint16_t> cuDraw(w, h);
  tdp::ManagedDeviceImage<float> cuD(wc, hc);
  tdp::ManagedDeviceImage<float> cuDu(wc, hc);
  tdp::ManagedDeviceImage<float> cuDv(wc, hc);
  tdp::ManagedDeviceImage<float> cuTmp(wc, hc);

  tdp::Camera<float> cam(Eigen::Vector4f(550,550,319.5,239.5)); 
  tdp::GeodesicHist<4> normalHist;
  // Stream and display video
  while(!pangolin::ShouldQuit())
  {
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glColor3f(1.0f, 1.0f, 1.0f);

    gui.NextFrames();

    pangolin::basetime t0 = pangolin::TimeNow();

    tdp::Image<uint16_t> dRaw;
    if (!gui.ImageD(dRaw)) continue;

    CopyImage(dRaw, cuDraw, cudaMemcpyHostToDevice);
    ConvertDepthGpu(cuDraw, cuD, 1e-4, 0.1, 4.);
    pangolin::basetime tDepth = pangolin::TimeNow();
    if (gui.verbose)
      std::cout << "depth conversion: " <<
        pangolin::TimeDiff_s(t0,tDepth) << std::endl;
    {
      pangolin::CudaScopedMappedPtr cuNbufp(cuNbuf);
      cudaMemset(*cuNbufp,0, hc*wc*sizeof(tdp::Vector3fda));
      tdp::Image<tdp::Vector3fda> cuN(wc, hc,
          wc*sizeof(tdp::Vector3fda), (tdp::Vector3fda*)*cuNbufp);
      Depth2Normals(cuD, cam, cuN);
      if (gui.show2DNormals) {
        CopyImage(cuN, n2Df, cudaMemcpyDeviceToHost);
        //ConvertPixelsMultiChannel<uint8_t,float,3>(n2D,n2Df,128,127);
        for(size_t y=0; y < n2Df.h_; ++y) {
          for(size_t x=0; x < n2Df.w_*3; ++x) {
            ((uint8_t*)n2D.RowPtr((int)y))[x] = floor(((float*)n2Df.RowPtr((int)y))[x]*128+127);
          }
        }
      }
      if (gui.computeHist) {
        if (gui.histFrameByFrame)
          normalHist.Reset();
        normalHist.ComputeGpu(cuN);
      }
    }
    cudaDeviceSynchronize();
    pangolin::basetime tNormal = pangolin::TimeNow();

    glEnable(GL_DEPTH_TEST);
    d_cam.Activate(s_cam);
    pangolin::glDrawAxis(1);
    if (gui.dispNormals) {
      pangolin::RenderVbo(cuNbuf);
    }
    if (gui.computeHist) {
      if (gui.dispGrid) {
        normalHist.geoGrid_.Render3D();
      }
      normalHist.Render3D(gui.histScale);
    }

    glLineWidth(1.5f);
    glDisable(GL_DEPTH_TEST);

    gui.ShowFrames();

    CopyImage(cuDu, debugA, cudaMemcpyDeviceToHost);
    CopyImage(cuDv, debugB, cudaMemcpyDeviceToHost);
    viewDebugA.SetImage(debugA);
    viewDebugB.SetImage(debugB);
    if (gui.show2DNormals) {
      viewN2D.SetImage(n2D);
    }

    // leave in pixel orthographic for slider to render.
    pangolin::DisplayBase().ActivatePixelOrthographic();
    if(video.IsRecording()) {
      pangolin::glRecordGraphic(pangolin::DisplayBase().v.w-14.0f,
          pangolin::DisplayBase().v.h-14.0f, 7.0f);
    }
    pangolin::FinishFrame();
  }
}


int main( int argc, char* argv[] )
{
  const std::string dflt_output_uri = "pango://video.pango";

  if( argc > 1 ) {
    const std::string input_uri = std::string(argv[1]);
    const std::string output_uri = (argc > 2) ? std::string(argv[2]) : dflt_output_uri;
    try{
      VideoViewer(input_uri, output_uri);
    } catch (pangolin::VideoException e) {
      std::cout << e.what() << std::endl;
    }
  }else{
    const std::string input_uris[] = {
      "dc1394:[fps=30,dma=10,size=640x480,iso=400]//0",
      "convert:[fmt=RGB24]//v4l:///dev/video0",
      "convert:[fmt=RGB24]//v4l:///dev/video1",
      "openni:[img1=rgb]//",
      "test:[size=160x120,n=1,fmt=RGB24]//"
        ""
    };

    std::cout << "Usage  : VideoViewer [video-uri]" << std::endl << std::endl;
    std::cout << "Where video-uri describes a stream or file resource, e.g." << std::endl;
    std::cout << "\tfile:[realtime=1]///home/user/video/movie.pvn" << std::endl;
    std::cout << "\tfile:///home/user/video/movie.avi" << std::endl;
    std::cout << "\tfiles:///home/user/seqiemce/foo%03d.jpeg" << std::endl;
    std::cout << "\tdc1394:[fmt=RGB24,size=640x480,fps=30,iso=400,dma=10]//0" << std::endl;
    std::cout << "\tdc1394:[fmt=FORMAT7_1,size=640x480,pos=2+2,iso=400,dma=10]//0" << std::endl;
    std::cout << "\tv4l:///dev/video0" << std::endl;
    std::cout << "\tconvert:[fmt=RGB24]//v4l:///dev/video0" << std::endl;
    std::cout << "\tmjpeg://http://127.0.0.1/?action=stream" << std::endl;
    std::cout << "\topenni:[img1=rgb]//" << std::endl;
    std::cout << std::endl;

    // Try to open some video device
    for(int i=0; !input_uris[i].empty(); ++i )
    {
      try{
        pango_print_info("Trying: %s\n", input_uris[i].c_str());
        VideoViewer(input_uris[i], dflt_output_uri);
        return 0;
      }catch(pangolin::VideoException) { }
    }
  }

  return 0;
}
