#pragma once
#include "Utils.h"


namespace zeta
{

	class Renderable
	{
	public:
		virtual ~Renderable() {}
		virtual void Render(ID3DX11Effect* effect, ID3DX11EffectPass* pass) = 0;
	};


	class StaticMeshRenderable 
		: public Renderable
	{
	public:
		StaticMeshRenderable();
		virtual ~StaticMeshRenderable();

		virtual void Render(ID3DX11Effect* effect, ID3DX11EffectPass* pass) override;
		
		void CreateVertexBuffer(size_t num_vert, const Vector3f* pos_data, const Vector3f* norm_data, const Vector2f* tc_data);
		void CreateIndexBuffer(size_t num_indice, const uint32_t* data);

		void CreateMtl(std::string albedo_tex, Vector3f albedo_clr, float metalness, float glossiness);

		void Destory();

		ID3D11InputLayout* D3DInputLayout(ID3DX11EffectPass* pass);

	private:
		struct VS_INPUT
		{
			Vector3f pos;
			Vector3f norm;
			Vector2f tc;
		};

		ID3D11BufferPtr d3d_vertex_buffer_;
		ID3D11BufferPtr d3d_index_buffer_;

		unsigned int num_indice_;

		std::vector<std::pair<ID3DX11EffectPass*, ID3D11InputLayoutPtr>> d3d_input_layouts_;
		
		struct TEX
		{
			ID3D11ResourcePtr d3d_tex_;
			ID3D11ShaderResourceViewPtr d3d_srv_;
		};
		
		TEX albedo_tex_;
		Vector3f albedo_clr_;
		float metalness_;
		float glossiness_;
	};


	class QuadRenderable 
		: public Renderable
	{
	public:
		QuadRenderable();
		virtual ~QuadRenderable();

		virtual void Render(ID3DX11Effect* effect, ID3DX11EffectPass* pass) override;

		void Destory();

		void EnsureVertexBuffer();

		ID3D11InputLayout* D3DInputLayout(ID3DX11EffectPass* pass);

	protected:
		ID3D11BufferPtr d3d_vertex_buffer_;

		std::vector<std::pair<ID3DX11EffectPass*, ID3D11InputLayoutPtr>> d3d_input_layouts_;
	};


	class SkyBoxRenderable 
		: public QuadRenderable
	{
	public:
		SkyBoxRenderable();
		virtual ~SkyBoxRenderable();

		void CreateCubeMap(std::string file_path);
		void CreateCompressedCubeMap(std::string file_path_y, std::string file_path_c);
		void SetCamera(CameraPtr cam);

		virtual void Render(ID3DX11Effect* effect, ID3DX11EffectPass* pass) override;

	private:
		ID3D11ResourcePtr d3d_tex_;
		ID3D11ShaderResourceViewPtr d3d_srv_;

		ID3D11ResourcePtr d3d_tex_y_;
		ID3D11ShaderResourceViewPtr d3d_srv_y_;

		ID3D11ResourcePtr d3d_tex_c_;
		ID3D11ShaderResourceViewPtr d3d_srv_c_;

		bool compressed_;

		CameraPtr cam_;
	};

}