// Zeta.cpp : 定义控制台应用程序的入口点。
//
// Zeta.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "Application.h"
#include <filesystem>
#include <assimp\config.h>
#include <assimp\cimport.h>
#include <assimp\postprocess.h>
#include <assimp\scene.h>
#include "Renderable.h"
#include "Camera.h"
#include "Light.h"
#include "Renderer.h"


using namespace zeta;


void GenerateCube(StaticMeshRenderablePtr r)
{
	static Vector3f verts[8] = {
		Vector3f(-1, -1, -1),
		Vector3f(1, -1, -1),
		Vector3f(1, 1, -1),
		Vector3f(-1, 1, -1),
		Vector3f(-1, -1, 1),
		Vector3f(1, -1, 1),
		Vector3f(1, 1, 1),
		Vector3f(-1, 1, 1),
	};

	std::vector<Vector3f> positions;
	std::vector<Vector3f> normals;
	std::vector<Vector2f> texcoords;
	std::vector<uint32_t> indices;

	auto draw_plane = [&](int a, int b, int c, int d) {
		Vector3f p1 = verts[a], p2 = verts[b], p3 = verts[c], p4 = verts[d];
		Vector3f norm = CrossProduct3((p3 - p2), (p1 - p2));

		float uv1 = 0.1f;
		float uv2 = 0.9f;
		Vector2f tc1(uv1, uv1), tc2(uv1, uv2), tc3(uv2, uv2), tc4(uv2, uv1);

		uint32_t index = (uint32_t)positions.size();
		positions.push_back(p1);
		positions.push_back(p2);
		positions.push_back(p3);
		normals.insert(normals.end(), 3, norm);
		texcoords.push_back(tc1);
		texcoords.push_back(tc2);
		texcoords.push_back(tc3);
		indices.push_back(index);
		indices.push_back(index + 1);
		indices.push_back(index + 2);

		index = (uint32_t)positions.size();
		positions.push_back(p3);
		positions.push_back(p4);
		positions.push_back(p1);
		normals.insert(normals.end(), 3, norm);
		texcoords.push_back(tc3);
		texcoords.push_back(tc4);
		texcoords.push_back(tc1);
		indices.push_back(index);
		indices.push_back(index + 1);
		indices.push_back(index + 2);
	};

	draw_plane(0, 3, 2, 1);
	draw_plane(4, 5, 6, 7);
	draw_plane(0, 1, 5, 4);
	draw_plane(1, 2, 6, 5);
	draw_plane(2, 3, 7, 6);
	draw_plane(0, 4, 7, 3);

	r->CreateVertexBuffer(positions.size(), positions.data(), normals.data(), texcoords.data());
	r->CreateIndexBuffer(indices.size(), indices.data());
}


void LoadAssimpStaticMesh(std::string file_path, float scale = 1, bool inverse_z = false, bool swap_yz = false)
{
	aiPropertyStore* props = aiCreatePropertyStore();
	aiSetImportPropertyInteger(props, AI_CONFIG_IMPORT_TER_MAKE_UVS, 1);
	aiSetImportPropertyFloat(props, AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, 80);
	aiSetImportPropertyInteger(props, AI_CONFIG_PP_SBP_REMOVE, 0);
	aiSetImportPropertyInteger(props, AI_CONFIG_GLOB_MEASURE_TIME, 1);

	unsigned int ppsteps = aiProcess_CalcTangentSpace | // calculate tangents and bitangents if possible
		aiProcess_JoinIdenticalVertices | // join identical vertices/ optimize indexing
		aiProcess_ValidateDataStructure | // perform a full validation of the loader's output
		aiProcess_ImproveCacheLocality | // improve the cache locality of the output vertices
		aiProcess_RemoveRedundantMaterials | // remove redundant materials
		aiProcess_FindDegenerates | // remove degenerated polygons from the import
		aiProcess_FindInvalidData | // detect invalid model data, such as invalid normal vectors
		aiProcess_GenUVCoords | // convert spherical, cylindrical, box and planar mapping to proper UVs
		aiProcess_TransformUVCoords | // preprocess UV transformations (scaling, translation ...)
		aiProcess_FindInstances | // search for instanced meshes and remove them by references to one master
		aiProcess_LimitBoneWeights | // limit bone weights to 4 per vertex
		aiProcess_OptimizeMeshes | // join small meshes, if possible;
		aiProcess_SplitByBoneCount | // split meshes with too many bones. Necessary for our (limited) hardware skinning shader
		0;

	aiScene const * scene = aiImportFileExWithProperties(file_path.c_str(),
		ppsteps | /* configurable pp steps */
		aiProcess_GenSmoothNormals | // generate smooth normal vectors if not existing
		aiProcess_SplitLargeMeshes | // split large, unrenderable meshes into submeshes
		aiProcess_Triangulate | // triangulate polygons with more than 3 edges
		aiProcess_ConvertToLeftHanded | // convert everything to D3D left handed space
		aiProcess_SortByPType | // make 'clean' meshes which consist of a single typ of primitives
		0,
		NULL,
		props);

	if (!scene)
	{
		printf("%s\n", aiGetErrorString());
		getchar();
		return;
	}

	auto pp = _FSPFX path(file_path).parent_path();

	for (unsigned int mi = 0; mi < scene->mNumMeshes; ++mi)
	{
		aiMesh const * mesh = scene->mMeshes[mi];

		std::string tex_path;
		Vector3f ka(0.2f, 0.2f, 0.2f);
		Vector3f kd(0.5f, 0.5f, 0.5f);
		Vector3f ks(0.7f, 0.7f, 0.7f);
		std::vector<Vector3f> pos_data;
		std::vector<Vector3f> norm_data;
		std::vector<Vector2f> tc_data;
		std::vector<uint32_t> indice_data;
		size_t num_vert = mesh->mNumVertices;

		auto mtl = scene->mMaterials[mesh->mMaterialIndex];

		unsigned int count = aiGetMaterialTextureCount(mtl, aiTextureType_DIFFUSE);
		if (count > 0)
		{
			aiString str;
			aiGetMaterialTexture(mtl, aiTextureType_DIFFUSE, 0, &str, 0, 0, 0, 0, 0, 0);
			auto pp2 = pp;
			pp2.append("/").append(str.C_Str());
			tex_path = pp2.string();
		}

		aiColor4D c4;
		if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_AMBIENT, &c4))
		{
			ka = Vector3f(c4.r, c4.g, c4.b);
		}
		if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_DIFFUSE, &c4))
		{
			kd = Vector3f(c4.r, c4.g, c4.b);
		}
		if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_SPECULAR, &c4))
		{
			ks = Vector3f(c4.r, c4.g, c4.b);
		}

		for (unsigned int fi = 0; fi < mesh->mNumFaces; ++fi)
		{
			if (3 == mesh->mFaces[fi].mNumIndices)
			{
				indice_data.push_back(mesh->mFaces[fi].mIndices[0]);
				indice_data.push_back(mesh->mFaces[fi].mIndices[1]);
				indice_data.push_back(mesh->mFaces[fi].mIndices[2]);
			}
		}

		pos_data.resize(num_vert);
		norm_data.resize(num_vert);
		tc_data.resize(num_vert);
		for (unsigned int vi = 0; vi < mesh->mNumVertices; ++vi)
		{
			pos_data[vi] = Vector3f(&mesh->mVertices[vi].x);
			pos_data[vi] = pos_data[vi] * scale;

			if (inverse_z)
			{
				pos_data[vi].z *= -1;
			}
			if (swap_yz)
			{
				std::swap(pos_data[vi].y, pos_data[vi].z);
			}

			if (mesh->mNormals)
			{
				norm_data[vi] = Vector3f(&mesh->mNormals[vi].x);

				if (inverse_z)
				{
					norm_data[vi].z *= -1;
				}
				if (swap_yz)
				{
					std::swap(norm_data[vi].y, norm_data[vi].z);
				}
			}

			if (mesh->mTextureCoords && mesh->mTextureCoords[0])
			{
				tc_data[vi] = Vector2f(&mesh->mTextureCoords[0][vi].x);
			}
		}

		StaticMeshRenderablePtr r = std::make_shared<StaticMeshRenderable>();
		r->CreateVertexBuffer(num_vert, pos_data.data(), norm_data.data(), tc_data.data());
		r->CreateIndexBuffer(indice_data.size(), indice_data.data());
		r->CreateMaterial(tex_path, ka, kd, ks);
		Renderer::Instance().AddRenderable(r);
	}
}


int main()
{
	try
	{
		int width = 1280;
		int height = 720;

		Application app;
		app.Create("Test", width, height);

		CameraPtr cam = std::make_shared<Camera>();
		Vector3f eye(2, 2, 2), at(0, 0, 0), up(0, 1, 0);
		cam->LookAt(eye, at, up);
		cam->Perspective(XM_PI / 4, (float)width / (float)height, 0.1f, 500);
		Renderer::Instance().SetCamera(cam);

		SkyBoxRenderablePtr skybox = std::make_shared<SkyBoxRenderable>();
		skybox->CreateCubeMap("Texture/CubeMap/BlueSky.dds");
		Renderer::Instance().SetSkyBox(skybox);

		AmbientLightPtr al = std::make_shared<AmbientLight>();
		al->color_ = Vector3f(0.1f, 0.1f, 0.1f);
		Renderer::Instance().SetAmbientLight(al);

		DirectionLightPtr dl = std::make_shared<DirectionLight>();
		dl->color_ = Vector3f(0.85f, 0.85f, 0.85f);
		dl->dir_ = Vector3f(cam->eye_pos_ - cam->look_at_);
		Renderer::Instance().AddDirectionLight(dl);

		LoadAssimpStaticMesh("Model/testObj/testObj.obj");

		app.Run();
	}
	catch (const std::exception& e)
	{
		std::string se = e.what();
	}

	return 0;
}
