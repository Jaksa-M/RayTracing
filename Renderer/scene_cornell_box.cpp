#include "scene_cornell_box.h"
#include <vector>
#include <cmath>
#include "hittable.h"
#include "hittable_list_custom_bvh.h"
#include "camera.h"
#include "material.h"
#include "matrix.h"
#include "transformations.h"
#include "mesh_utils.h"
#include "imgui/imgui.h"
#include <GLFW/glfw3.h>
#include "context.h"
#include "texture_loader.h"

SceneCornellBox::SceneCornellBox() {}

void SceneCornellBox::initialize() {
    world_ = std::make_unique<HittableListCustomBVH>();

    // Initialization of cameras
    std::unique_ptr<Camera> cam1 = std::make_unique<Camera>("initial cam");
    cam1->setInitalValues();
    cameras_.push_back(std::move(cam1));

	prev_BVH_technique_ = context.settings->BVH_technique;

	TextureLoader tex_loader("../Resources/textures/san_giuseppe_bridge.hdr");
    if (!tex_loader.load()) {
        std::cerr << "ERROR: Could not load background texture file.\n";
    }
    TexDescription desc(tex_loader.getImageWidth(), tex_loader.getImageHeight(), tex_loader.getFormat());
    background_texture_ = std::make_shared<Texture>(tex_loader.getData(), desc);
    for (uint32 i = 0; i < cameras_.size(); i++) {
        cameras_[i]->setBackgroundTexture(background_texture_);
    }

	auto mat_yellow = std::make_shared<Lambertian>(color(0.8f, 0.8f, 0.0f));
    auto mat_green = std::make_shared<Lambertian>(color(0.0f, 1.0f, 0.0f));
    auto mat_red = std::make_shared<Lambertian>(color(1.0f, 0.0f, 0.0f));
    auto mat_white = std::make_shared<Lambertian>(color(0.8f, 0.8f, 0.8f));

	// Add a diffuse white material with emission
	
    color diffuse_col(0.5f, 0.5f, 0.5f);
    color emissive_col(15.0f, 15.0f, 15.0f);
    std::shared_ptr<Texture> emissive_tex = std::make_shared<Texture>(emissive_col);
    std::shared_ptr<Texture> diffuse_tex = std::make_shared<Texture>(diffuse_col);
    
    auto mat_light = std::make_shared<Lambertian>(diffuse_tex, nullptr, emissive_tex);


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
	m = transformation::create_translation_matrix(vec3(0.99f, 1.0f, 0.0f)) *
		transformation::create_rotation_matrix(0.0f, 0.0f, -90.0f * (3.14159f / 180.0f)) *
		transformation::create_scaling_matrix(2.0f, 2.0f, 2.0f);
	rect_mesh_right_->setTransformationMatrix(m);

	// Ceiling light panel (diffuse + emissive)
    rect_mesh_light_ = MeshUtils::GenerateTriangleRectangle(context, mat_light, 2, 2);
    m = transformation::create_translation_matrix(vec3(0.0f, 1.98f, -0.25f)) * // position just below ceiling
        transformation::create_rotation_matrix(-180.0f * (3.14159f / 180.0f), 0.0f, 0.0f) *
        transformation::create_scaling_matrix(0.5f, 0.5f, 0.5f); // size of light source
    rect_mesh_light_->setTransformationMatrix(m);



	std::vector<MeshHandle> meshes;
    meshes.push_back(rect_prism_mesh_->getMeshHandle());
    meshes.push_back(cube_mesh_->getMeshHandle());
    meshes.push_back(rect_mesh_back_->getMeshHandle());
    meshes.push_back(rect_mesh_top_->getMeshHandle());
    meshes.push_back(rect_mesh_bottom_->getMeshHandle());
    meshes.push_back(rect_mesh_left_->getMeshHandle());
    meshes.push_back(rect_mesh_right_->getMeshHandle());
    meshes.push_back(rect_mesh_light_->getMeshHandle());
    rt_meshes_.push_back(std::move(rect_prism_mesh_));
    rt_meshes_.push_back(std::move(cube_mesh_));
    rt_meshes_.push_back(std::move(rect_mesh_back_));
    rt_meshes_.push_back(std::move(rect_mesh_top_));
    rt_meshes_.push_back(std::move(rect_mesh_bottom_));
    rt_meshes_.push_back(std::move(rect_mesh_left_));
    rt_meshes_.push_back(std::move(rect_mesh_right_));
    rt_meshes_.push_back(std::move(rect_mesh_light_));


	//sphere1_ = MeshUtils::GenerateIcosphere(context, mat_red, 4);
 //   matrix4x4 m = transformation::create_translation_matrix(vec3(-2.0f, 0.0f, 0.0f));
 //   sphere1_->setTransformationMatrix(m);

 //   sphere2_ = std::make_shared<RTMesh>(context, sphere1_->getMeshHandle(), mat_yellow);
 //   m = transformation::create_translation_matrix(vec3(0.0f, 0.0f, 0.0f));
 //   sphere2_->setTransformationMatrix(m);

 //   sphere3_ = std::make_shared<RTMesh>(context, sphere1_->getMeshHandle(), mat_green);
 //   m = transformation::create_translation_matrix(vec3(2.0f, 0.0f, 0.0f));
 //   sphere3_->setTransformationMatrix(m);

 //   sphere4_ = std::make_shared<RTMesh>(context, sphere1_->getMeshHandle(), mat_white);
 //   m = transformation::create_translation_matrix(vec3(5.0f, 0.0f, 0.0f));
 //   sphere4_->setTransformationMatrix(m);
 //   meshes.push_back(sphere1_->getMeshHandle());
 //   meshes.push_back(sphere2_->getMeshHandle());
 //   meshes.push_back(sphere3_->getMeshHandle());
 //   meshes.push_back(sphere4_->getMeshHandle());
 //   rt_meshes_.push_back(std::move(sphere1_));
 //   rt_meshes_.push_back(std::move(sphere2_));
 //   rt_meshes_.push_back(std::move(sphere3_));
 //   rt_meshes_.push_back(std::move(sphere4_));



    for (uint32 i = 0; i < rt_meshes_.size(); i++) {
        world_->add(rt_meshes_[i]);
    }

	sendMeshDataToGPU();

	static_cast<HittableListCustomBVH*>(world_.get())->buildTLAS(context.bvh_manager, meshes, rt_meshes_);

	initShader();
}

void SceneCornellBox::update(uint32 display_w, uint32 display_h) {
	if (prev_BVH_technique_ != context.settings->BVH_technique) {
        world_->clear();
		initialize();
	}
    cameras_[active_camera_]->image_width = display_w;
    cameras_[active_camera_]->image_height = display_h;
	// Update RTMesh vertices/indices/uvs/normals once per frame
    world_->update();
}

void SceneCornellBox::initShader() {
	shader_prog_ = std::make_unique<Shader>("../ShaderFiles/shader_bounding_box.vs.txt", "../ShaderFiles/shader_bounding_box.fs.txt");
}

void SceneCornellBox::drawBVH() {
	if (context.settings->selected_option != -1) {
        bounding_boxes_.resize(world_->getSize());

		for (uint32 i = 0; i < world_->getSize(); i++) {  // Drawing BVH tree or leaves
            std::shared_ptr<Hittable> object = world_->getObject(i);
			RTMesh* rtMesh = dynamic_cast<RTMesh*>(object.get());

			if (rtMesh) { // If the cast succeeds, the object is of type RTMesh
				if (context.settings->selected_option == 0) { // Drawing whole tree
                    rtMesh->drawBVHTree(bounding_boxes_, i, shader_prog_, *cameras_[0]);
				}
				else if (context.settings->selected_option == 1) { // Drawing only leaves
                    rtMesh->drawBVHLeaves(bounding_boxes_, i, shader_prog_, *cameras_[0]);
				}
			}
		}
	}
}
