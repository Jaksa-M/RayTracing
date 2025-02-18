#include "scene_cornell_box.h"
#include <vector>
#include <cmath>
#include <memory>
#include "hittable.h"
#include "hittable_list.h"
#include "camera.h"
#include "material.h"
#include "matrix.h"
#include "transformations.h"
#include "color.h"
#include "mesh_utils.h"
#include "imgui/imgui.h"
#include <GLFW/glfw3.h>
#include "context.h"
#include <chrono> // Time

SceneCornellBox::SceneCornellBox() {

}

void SceneCornellBox::initialize() {
	prev_BVH_technique_ = context.settings->BVH_technique;

	auto mat_yellow = std::make_shared<lambertian>(color(0.8f, 0.8f, 0.0f));
	auto mat_green = std::make_shared<lambertian>(color(0.0f, 1.0f, 0.0f));
	auto mat_red = std::make_shared<lambertian>(color(1.0f, 0.0f, 0.0f));
	auto mat_white = std::make_shared<lambertian>(color(0.8f, 0.8f, 0.8f));

	// Creating meshes and their transformation matrices
	rect_prism_mesh_ = MeshUtils::GenerateTriangleCube(context, mat_yellow, 2);
	matrix4x4 m = transformation::create_rotation_matrix(0.0f, 30.0f * (3.14159f / 180.0f), 0.0f) *
		transformation::create_translation_matrix(vec3(-0.1f, 1.0f, -0.2f)) *
		transformation::create_scaling_matrix(0.7f, 1.2f, 0.7f);
	rect_prism_mesh_->setTransformationMatrix(m);

	cube_mesh_ = std::make_shared<RTMesh>(context, rect_prism_mesh_->getMeshHandle(), mat_yellow);
	m = transformation::create_rotation_matrix(0.0f, 40.0f * (3.14159f / 180.0f), 0.0f) *
		transformation::create_translation_matrix(vec3(-0.05f, 0.6f, 0.4f)) *
		transformation::create_scaling_matrix(0.5f, 0.5f, 0.5f);
	cube_mesh_->setTransformationMatrix(m);

	rect_mesh_back_ = MeshUtils::GenerateTriangleRectangle(context, mat_white, 2, 2);
	m = transformation::create_translation_matrix(vec3(0.0f, 1.0f, -0.99f)) *
		transformation::create_rotation_matrix(90.0f * (3.14159f / 180.0f), 0.0f, 0.0f) *
		transformation::create_scaling_matrix(2.0f, 2.0f, 2.0f);
	rect_mesh_back_->setTransformationMatrix(m);

	rect_mesh_top_ = std::make_shared<RTMesh>(context, rect_mesh_back_->getMeshHandle(), mat_white);
	m = transformation::create_translation_matrix(vec3(0.0f, 1.99f, 0.0f)) *
		transformation::create_rotation_matrix(-180.0f * (3.14159f / 180.0f), 0.0f, 0.0f) *
		transformation::create_scaling_matrix(2.0f, 2.0f, 2.0f);
	rect_mesh_top_->setTransformationMatrix(m);

	rect_mesh_bottom_ = std::make_shared<RTMesh>(context, rect_mesh_back_->getMeshHandle(), mat_white);
	m = transformation::create_translation_matrix(vec3(0.0f, 0.01f, 0.0f)) *
		transformation::create_scaling_matrix(2.0f, 2.0f, 2.0f);
	rect_mesh_bottom_->setTransformationMatrix(m);

	rect_mesh_left_ = std::make_shared<RTMesh>(context, rect_mesh_back_->getMeshHandle(), mat_red);
	m = transformation::create_translation_matrix(vec3(-0.99f, 1.0f, 0.0f)) *
		transformation::create_rotation_matrix(0.0f, 0.0f, 90.0f * (3.14159f / 180.0f)) *
		transformation::create_scaling_matrix(2.0f, 2.0f, 2.0f);
	rect_mesh_left_->setTransformationMatrix(m);

	rect_mesh_right_ = std::make_shared<RTMesh>(context, rect_mesh_back_->getMeshHandle(), mat_green);
	m = transformation::create_translation_matrix(vec3(0.99f, 1.f, 0.0f)) *
		transformation::create_rotation_matrix(0.0f, 0.0f, -90.0f * (3.14159f / 180.0f)) *
		transformation::create_scaling_matrix(2.0f, 2.0f, 2.0f);
	rect_mesh_right_->setTransformationMatrix(m);

	world_.add(rect_prism_mesh_);
	world_.add(cube_mesh_);
	world_.add(rect_mesh_top_);
	world_.add(rect_mesh_bottom_);
	world_.add(rect_mesh_left_);
	world_.add(rect_mesh_right_);
	world_.add(rect_mesh_back_);
 //   auto start_time = std::chrono::high_resolution_clock::now();  // Start timing
	//
 //   obj_loader_ = std::make_unique<ObjLoader>("Resources/teapot.obj");
 //   if (!obj_loader_->load(context, mat_green)) std::cout << "custom mesh failed to load" << std::endl;

 //   std::span<MeshHandle> meshes = obj_loader_->getMeshes();
 //   for (std::uint32_t i = 0; i < meshes.size(); i++) {
 //       rt_meshes_.push_back(std::make_shared<RTMesh>(context, meshes[i], mat_green));
 //   }
 //   m = //matrix4x4::identity();
 //   transformation::create_scaling_matrix(0.05f, 0.05f, 0.05f);
	//for (std::uint32_t i = 0; i < rt_meshes_.size(); i++) {
 //       rt_meshes_[i]->setTransformationMatrix(m);
 //       world_.add(rt_meshes_[i]);
 //   }
 //   auto end_time = std::chrono::high_resolution_clock::now();  // End timing
 //   std::chrono::duration<double> elapsed = end_time - start_time;

 //   std::cout << "Execution time: " << elapsed.count() << " seconds" << std::endl;
	initShader();
}

std::vector<unsigned char> SceneCornellBox::update(int display_w, int display_h, Camera& cam) {
	if (prev_BVH_technique_ != context.settings->BVH_technique) {
		world_.clear();
		initialize();
	}

	std::vector<unsigned char> image_data;
	image_data = cam.render(world_, image_data_acc_, *(context.settings));
	return image_data;
}

void SceneCornellBox::initShader() {
	shader_prog_ = std::make_unique<Shader>("ShaderFiles/shader_bounding_box.vs.txt", "ShaderFiles/shader_bounding_box.fs.txt");
}

void SceneCornellBox::drawBVH(Camera& cam) {
	if (context.settings->selected_option != -1) {
		bounding_boxes_.resize(world_.objects_.size());

		for (int i = 0; i < world_.objects_.size(); i++) { // Drawing BVH tree or leaves
			auto& object = world_.objects_[i];
			RTMesh* rtMesh = dynamic_cast<RTMesh*>(object.get());

			if (rtMesh) { // If the cast succeeds, the object is of type RTMesh
				if (context.settings->selected_option == 0) { // Drawing whole tree
					rtMesh->drawBVHTree(bounding_boxes_, i, shader_prog_, cam);
				}
				else if (context.settings->selected_option == 1) { // Drawing only leaves
					rtMesh->drawBVHLeaves(bounding_boxes_, i, shader_prog_, cam);
				}
			}
		}
	}
}
