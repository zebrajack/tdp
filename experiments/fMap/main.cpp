/* Copyright (c) 2016, Julian Straub <jstraub@csail.mit.edu> Licensed
 * under the MIT license. See the license file LICENSE.
 */
#include <iostream>
#include <cmath>
#include <complex>
#include <vector>
#include <cstdlib>


#include <pangolin/pangolin.h>
#include <pangolin/video/video_record_repeat.h>
#include <pangolin/gl/gltexturecache.h>
#include <pangolin/gl/glpixformat.h>
#include <pangolin/handler/handler_image.h>
#include <pangolin/utils/file_utils.h>
#include <pangolin/utils/timer.h>
#include <pangolin/gl/gl.h>
#include <pangolin/gl/glsl.h>
#include <pangolin/gl/glvbo.h>
#include <pangolin/gl/gldraw.h>
#include <pangolin/image/image_io.h>

#include <tdp/eigen/dense.h>
#include <Eigen/Eigenvalues>
#include <Eigen/Sparse>

#include <tdp/preproc/depth.h>
#include <tdp/preproc/pc.h>
#include <tdp/camera/camera.h>
#ifdef CUDA_FOUND
#include <tdp/preproc/normals.h>
#endif

#include <tdp/io/tinyply.h>
#include <tdp/gl/shaders.h>
#include <tdp/gl/gl_draw.h>

#include <tdp/gui/gui.hpp>
#include <tdp/gui/quickView.h>

#include <tdp/nn/ann.h>
#include <tdp/manifold/S.h>
#include <tdp/manifold/SE3.h>
#include <tdp/data/managed_image.h>

#include <tdp/utils/status.h>
#include <tdp/utils/timer.hpp>
#include <tdp/eigen/std_vector.h>


#include <tdp/laplace_beltrami/laplace_beltrami.h>



int main( int argc, char* argv[] ){

  tdp::ManagedHostImage<tdp::Vector3fda> pc_s(1000,1);
  tdp::ManagedHostImage<tdp::Vector3fda> ns_s(1000,1);

  tdp::ManagedHostImage<tdp::Vector3fda> pc_t(1000,1);
  tdp::ManagedHostImage<tdp::Vector3fda> ns_t(1000,1);

  if (argc > 1) {
      const std::string input = std::string(argv[1]);
      std::cout << "input pc: " << input << std::endl;
      tdp::LoadPointCloud(input, pc_s, ns_s);
  } else {
      std::srand(101);
      GetSphericalPc(pc_s);
      std::srand(200);
      GetSphericalPc(pc_t);
      //GetCylindricalPc(pc);
  }

  // build kd tree
  tdp::ANN ann_s, ann_t;
  ann_s.ComputeKDtree(pc_s);
  ann_t.ComputeKDtree(pc_t);

  // Create OpenGL window - guess sensible dimensions
  int menue_w = 180;
  pangolin::CreateWindowAndBind( "GuiBase", 1200+menue_w, 800);
  // current frame in memory buffer and displaying.
  pangolin::CreatePanel("ui").SetBounds(0.,1.,0.,pangolin::Attach::Pix(menue_w));
  // Assume packed OpenGL data unless otherwise specified
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  // setup container
  pangolin::View& container = pangolin::Display("container");
  container.SetLayout(pangolin::LayoutEqual)
    .SetBounds(0., 1.0, pangolin::Attach::Pix(menue_w), 1.0);
  // Define Camera Render Object (for view / scene browsing)
  pangolin::OpenGlRenderState s_cam(
      pangolin::ProjectionMatrix(640,480,420,420,320,240,0.1,1000),
      pangolin::ModelViewLookAt(0,0.5,-3, 0,0,0, pangolin::AxisNegY)
      );
  // Add named OpenGL viewport to window and provide 3D Handler
  pangolin::View& viewPc = pangolin::CreateDisplay()
    .SetHandler(new pangolin::Handler3D(s_cam));
  container.AddDisplay(viewPc);
  pangolin::View& viewPc_t = pangolin::CreateDisplay()
    .SetHandler(new pangolin::Handler3D(s_cam));
  container.AddDisplay(viewPc_t);


  // use those OpenGL buffers
  pangolin::GlBuffer vbo,vbo_t, valuebo_s,valuebo_t;
  vbo.Reinitialise(pangolin::GlArrayBuffer, pc_s.Area(),  GL_FLOAT, 3, GL_DYNAMIC_DRAW);
  vbo.Upload(pc_s.ptr_, pc_s.SizeBytes(), 0);
  vbo_t.Reinitialise(pangolin::GlArrayBuffer, pc_t.Area(),  GL_FLOAT, 3, GL_DYNAMIC_DRAW);
  vbo_t.Upload(pc_t.ptr_, pc_t.SizeBytes(), 0);

  // Add variables to pangolin GUI
  pangolin::Var<bool> showFMap("ui.show fMap", true, false);
  // pangolin::Var<int> pcOption("ui. pc option", 0, 0,1);
  // variables for KNN
  pangolin::Var<int> knn("ui.knn",30,1,100);
  pangolin::Var<float> eps("ui.eps", 1e-6 ,1e-7, 1e-5);
  pangolin::Var<float> alpha("ui. alpha", 0.01, 0.005, 0.3); //variance of rbf kernel
  pangolin::Var<int> numEv("ui.numEv",3,1,300);

  // viz color coding
  pangolin::Var<float>minVal("ui. min Val",-0.71,-1,0);
  pangolin::Var<float>maxVal("ui. max Val",0.01,1,0);
  float minVal_t, maxVal_t;

  Eigen::SparseMatrix<float> L_s(pc_s.Area(), pc_s.Area());
  Eigen::SparseMatrix<float> L_t(pc_t.Area(), pc_t.Area());
  tdp::eigen_vector<Eigen::VectorXf> basis_s(numEv, Eigen::VectorXf::Zero(L_s.rows()));
  tdp::eigen_vector<Eigen::VectorXf> basis_t(numEv, Eigen::VectorXf::Zero(L_s.rows()));

  // Stream and display video
  while(!pangolin::ShouldQuit())
  {
    // clear the OpenGL render buffers
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glColor3f(1.0f, 1.0f, 1.0f);

    if (pangolin::Pushed(showFMap) || knn.GuiChanged() || alpha.GuiChanged() ||
            numEv.GuiChanged()){
      std::cout << "Running fMap..." << std::endl;

      // get Laplacian operator and its eigenvectors
      tdp::Timer t0;
      L_s = tdp::getLaplacian(pc_s, ann_s, knn, eps, alpha);
      L_t = tdp::getLaplacian(pc_t, ann_t, knn, eps, alpha);
      t0.toctic("GetLaplacians");
      tdp::getLaplacianEvectors(L_s, numEv, basis_s);
      tdp::getLaplacianEvectors(L_t, numEv, basis_t);
      t0.toctic("GetEigenVectors");

      // color-coding on the surface
      valuebo_s.Reinitialise(pangolin::GlArrayBuffer, basis_s[1].rows(),GL_FLOAT,1, GL_DYNAMIC_DRAW);
      valuebo_s.Upload(&basis_s[1](0), sizeof(float)*basis_s[1].rows(), 0);
      std::cout << basis_s[1].minCoeff() << " " << basis_s[1].maxCoeff() << std::endl;
      minVal = basis_s[1].minCoeff()-1e-3;
      maxVal = basis_s[1].maxCoeff();

      valuebo_t.Reinitialise(pangolin::GlArrayBuffer, basis_t[1].rows(),GL_FLOAT,1, GL_DYNAMIC_DRAW);
      valuebo_t.Upload(&basis_t[1](0), sizeof(float)*basis_t[1].rows(), 0);
      std::cout << basis_t[1].minCoeff() << " " << basis_t[1].maxCoeff() << std::endl;
      minVal_t = basis_t[1].minCoeff()-1e-3;
      maxVal_t = basis_t[1].maxCoeff();

      std::cout << "<--DONE fMap-->" << std::endl;
    }


    // Draw 3D stuff
    glEnable(GL_DEPTH_TEST);
    glColor3f(1.0f, 1.0f, 1.0f);
    if (viewPc.IsShown()) {
      viewPc.Activate(s_cam);
      pangolin::glDrawAxis(0.1);

      glPointSize(2.);
      glColor3f(1.0f, 1.0f, 0.0f);

      // renders the vbo with colors from valuebo
      auto& shader = tdp::Shaders::Instance()->valueShader_;
      shader.Bind();
      shader.SetUniform("P",  s_cam.GetProjectionMatrix());
      shader.SetUniform("MV", s_cam.GetModelViewMatrix());
      shader.SetUniform("minValue", minVal);
      shader.SetUniform("maxValue", maxVal);
      valuebo_s.Bind();
      glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, 0);
      vbo.Bind();
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

      glEnableVertexAttribArray(0);
      glEnableVertexAttribArray(1);
      glPointSize(4.);
      glDrawArrays(GL_POINTS, 0, vbo.num_elements);
      shader.Unbind();
      glDisableVertexAttribArray(1);
      valuebo_s.Unbind();
      glDisableVertexAttribArray(0);
      vbo.Unbind();
    }

    if (viewPc_t.IsShown()){
        viewPc_t.Activate(s_cam);
        pangolin::glDrawAxis(0.1);

        glPointSize(2.);
        glColor3f(1.0f, 1.0f, 0.0f);

        // renders the vbo with colors from valuebo
        auto& shader_t = tdp::Shaders::Instance()->valueShader_;
        shader_t.Bind();
        shader_t.SetUniform("P",  s_cam.GetProjectionMatrix());
        shader_t.SetUniform("MV", s_cam.GetModelViewMatrix());
        shader_t.SetUniform("minValue", minVal_t);
        shader_t.SetUniform("maxValue", maxVal_t);
        valuebo_t.Bind();
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, 0);
        vbo_t.Bind();
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glPointSize(4.);
        glDrawArrays(GL_POINTS, 0, vbo_t.num_elements);
        shader_t.Unbind();
        glDisableVertexAttribArray(1);
        valuebo_t.Unbind();
        glDisableVertexAttribArray(0);
        vbo_t.Unbind();

    }

    glDisable(GL_DEPTH_TEST);
    // leave in pixel orthographic for slider to render.
    pangolin::DisplayBase().ActivatePixelOrthographic();
    // finish this frame
    pangolin::FinishFrame();
  }

  std::cout << "good morning!" << std::endl;
  return 0;
}
