#include "stdafx.h"
#include "Renderable.h"
#include "Renderer.h"
#include "DDSTextureLoader.h"
#include "Camera.h"


namespace zeta
{

	void StaticMeshRenderable::CreateVertexBuffer(size_t num_vert,
		const Vector3f* pos_data,
		const Vector3f* norm_data,
		const Vector2f* tc_data)
	{
		std::vector<VS_INPUT> vs_inputs(num_vert);
		for (size_t i = 0; i != num_vert; i++)
		{
			vs_inputs[i].pos = pos_data[i];
			vs_inputs[i].norm = norm_data[i];
			vs_inputs[i].tc = tc_data[i];
		}

		D3D11_BUFFER_DESC buffer_desc;
		buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		buffer_desc.ByteWidth = sizeof(VS_INPUT) * (UINT)num_vert;
		buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		buffer_desc.CPUAccessFlags = 0;
		buffer_desc.MiscFlags = 0;
		buffer_desc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA buffer_data;
		buffer_data.pSysMem = vs_inputs.data();
		buffer_data.SysMemPitch = 0;
		buffer_data.SysMemSlicePitch = 0;

		ID3D11Buffer* d3d_buffer = nullptr;
		THROW_FAILED(Renderer::Instance().D3DDevice()->CreateBuffer(&buffer_desc, &buffer_data, &d3d_buffer));
		d3d_vertex_buffer_ = MakeCOMPtr(d3d_buffer);
	}

	void StaticMeshRenderable::CreateIndexBuffer(size_t num_indice, const uint32_t* data)
	{
		D3D11_BUFFER_DESC buffer_desc;
		buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		buffer_desc.ByteWidth = sizeof(uint32_t) * (UINT)num_indice;
		buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		buffer_desc.CPUAccessFlags = 0;
		buffer_desc.MiscFlags = 0;
		buffer_desc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA buffer_data;
		buffer_data.pSysMem = data;
		buffer_data.SysMemPitch = 0;
		buffer_data.SysMemSlicePitch = 0;

		ID3D11Buffer* d3d_index_buffer = nullptr;
		THROW_FAILED(Renderer::Instance().D3DDevice()->CreateBuffer(&buffer_desc, &buffer_data, &d3d_index_buffer));
		d3d_index_buffer_ = MakeCOMPtr(d3d_index_buffer);

		num_indice_ = (UINT)num_indice;
	}

	void StaticMeshRenderable::CreateMtl(std::string albedo_tex, Vector3f albedo_clr, float metalness, float glossiness)
	{
		if (!albedo_tex.empty())
		{
			std::wstring wfile_path = ToW(albedo_tex);

			ID3D11Resource* d3d_tex_res = nullptr;
			ID3D11ShaderResourceView* d3d_tex_srv = nullptr;

			if (SUCCEEDED(
				CreateDDSTextureFromFileEx(Renderer::Instance().D3DDevice(), nullptr, wfile_path.c_str(), 0,
				D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, true,
				&d3d_tex_res, &d3d_tex_srv))
				)
			{
				albedo_tex_.d3d_tex_ = MakeCOMPtr(d3d_tex_res);
				albedo_tex_.d3d_srv_ = MakeCOMPtr(d3d_tex_srv);
			}
		}

		albedo_clr_ = albedo_clr;
		metalness_ = metalness;
		glossiness_ = glossiness;
	}

	ID3D11InputLayout* StaticMeshRenderable::D3DInputLayout(ID3DX11EffectPass* pass)
	{
		for (auto i = d3d_input_layouts_.begin(); i != d3d_input_layouts_.end(); i++)
		{
			if (i->first == pass)
			{
				return i->second.get();
			}
		}

		const D3D11_INPUT_ELEMENT_DESC d3d_elems_descs[] =
		{
			{ "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		D3DX11_PASS_DESC pass_desc;
		THROW_FAILED(pass->GetDesc(&pass_desc));

		ID3D11InputLayout* d3d_input_layout = nullptr;
		THROW_FAILED(Renderer::Instance().D3DDevice()->CreateInputLayout(d3d_elems_descs, (UINT)std::size(d3d_elems_descs),
			pass_desc.pIAInputSignature, pass_desc.IAInputSignatureSize, &d3d_input_layout));

		d3d_input_layouts_.emplace_back(pass, MakeCOMPtr(d3d_input_layout));

		return d3d_input_layout;
	}

	StaticMeshRenderable::StaticMeshRenderable()
	{
		num_indice_ = 0;
	}

	StaticMeshRenderable::~StaticMeshRenderable()
	{
		this->Destory();
	}

	void StaticMeshRenderable::Destory()
	{
		d3d_input_layouts_.clear();
		d3d_vertex_buffer_.reset();
		d3d_index_buffer_.reset();

		albedo_tex_.d3d_tex_.reset();
		albedo_tex_.d3d_srv_.reset();
	}

	void StaticMeshRenderable::Render(ID3DX11Effect* effect, ID3DX11EffectPass* pass)
	{
		//Material
		if (albedo_tex_.d3d_srv_)
		{
			Vector3f albedo_clr(0.58f, 0.58f, 0.58f);
	
			SetEffectVar(effect, "g_albedo_tex", albedo_tex_.d3d_srv_.get());
			SetEffectVar(effect, "g_albedo_map_enabled", true);
			SetEffectVar(effect, "g_albedo_clr", albedo_clr);
		}
		else
		{
			Vector3f albedo_clr = SRGBToLinear(albedo_clr_);

			SetEffectVar(effect, "g_albedo_map_enabled", false);
			SetEffectVar(effect, "g_albedo_clr", albedo_clr);
		}

		SetEffectVar(effect, "g_metalness_clr", Vector2f(metalness_, 0));
		SetEffectVar(effect, "g_glossiness_clr", Vector2f(glossiness_, 0));

		//Vertex buffer and index buffer
		std::array<ID3D11Buffer*, 1> buffers = {
			d3d_vertex_buffer_.get()
		};

		std::array<UINT, 1> strides = {
			sizeof(VS_INPUT)
		};

		std::array<UINT, 1> offsets = {
			0
		};

		Renderer::Instance().D3DContext()->IASetVertexBuffers(0, 1, buffers.data(), strides.data(), offsets.data());

		Renderer::Instance().D3DContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		Renderer::Instance().D3DContext()->IASetIndexBuffer(d3d_index_buffer_.get(), DXGI_FORMAT_R32_UINT, 0);

		Renderer::Instance().D3DContext()->IASetInputLayout(this->D3DInputLayout(pass));

		pass->Apply(0, Renderer::Instance().D3DContext());

		Renderer::Instance().D3DContext()->DrawIndexed(num_indice_, 0, 0);
	}

	QuadRenderable::QuadRenderable()
	{

	}

	QuadRenderable::~QuadRenderable()
	{
		this->Destory();
	}

	void QuadRenderable::Render(ID3DX11Effect* effect, ID3DX11EffectPass* pass)
	{
		this->EnsureVertexBuffer();
		
		//Vertex buffer and index buffer
		std::array<ID3D11Buffer*, 1> buffers = {
			d3d_vertex_buffer_.get()
		};

		std::array<UINT, 1> strides = {
			sizeof(Vector3f)
		};

		std::array<UINT, 1> offsets = {
			0
		};

		Renderer::Instance().D3DContext()->IASetVertexBuffers(0, 1, buffers.data(), strides.data(), offsets.data());

		Renderer::Instance().D3DContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		Renderer::Instance().D3DContext()->IASetInputLayout(this->D3DInputLayout(pass));

		pass->Apply(0, Renderer::Instance().D3DContext());

		Renderer::Instance().D3DContext()->Draw(4, 0);
	}

	void QuadRenderable::EnsureVertexBuffer()
	{
		if (!d3d_vertex_buffer_)
		{
			Vector3f vs_inputs[] =
			{
				Vector3f(-1, +1, 1),
				Vector3f(+1, +1, 1),
				Vector3f(-1, -1, 1),
				Vector3f(+1, -1, 1)
			};

			D3D11_BUFFER_DESC buffer_desc;
			buffer_desc.Usage = D3D11_USAGE_DEFAULT;
			buffer_desc.ByteWidth = sizeof(Vector3f) * (UINT)std::size(vs_inputs);
			buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			buffer_desc.CPUAccessFlags = 0;
			buffer_desc.MiscFlags = 0;
			buffer_desc.StructureByteStride = 0;

			D3D11_SUBRESOURCE_DATA buffer_data;
			buffer_data.pSysMem = vs_inputs;
			buffer_data.SysMemPitch = 0;
			buffer_data.SysMemSlicePitch = 0;

			ID3D11Buffer* d3d_buffer = nullptr;
			THROW_FAILED(Renderer::Instance().D3DDevice()->CreateBuffer(&buffer_desc, &buffer_data, &d3d_buffer));
			d3d_vertex_buffer_ = MakeCOMPtr(d3d_buffer);
		}
	}

	void QuadRenderable::Destory()
	{
		d3d_input_layouts_.clear();
	}

	ID3D11InputLayout* QuadRenderable::D3DInputLayout(ID3DX11EffectPass* pass)
	{
		for (auto i = d3d_input_layouts_.begin(); i != d3d_input_layouts_.end(); i++)
		{
			if (i->first == pass)
			{
				return i->second.get();
			}
		}

		const D3D11_INPUT_ELEMENT_DESC d3d_elems_descs[] =
		{
			{ "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		D3DX11_PASS_DESC pass_desc;
		THROW_FAILED(pass->GetDesc(&pass_desc));

		ID3D11InputLayout* d3d_input_layout = nullptr;
		THROW_FAILED(Renderer::Instance().D3DDevice()->CreateInputLayout(d3d_elems_descs, (UINT)std::size(d3d_elems_descs),
			pass_desc.pIAInputSignature, pass_desc.IAInputSignatureSize, &d3d_input_layout));

		d3d_input_layouts_.emplace_back(pass, MakeCOMPtr(d3d_input_layout));

		return d3d_input_layout;
	}

	SkyBoxRenderable::SkyBoxRenderable()
	{

	}
	
	SkyBoxRenderable::~SkyBoxRenderable()
	{

	}

	void SkyBoxRenderable::CreateCubeMap(std::string file_path)
	{
		if (!file_path.empty())
		{
			std::wstring wfile_path = ToW(file_path);

			ID3D11Resource* d3d_tex_res = nullptr;
			ID3D11ShaderResourceView* d3d_tex_srv = nullptr;
			if (SUCCEEDED(
				CreateDDSTextureFromFileEx(Renderer::Instance().D3DDevice(), nullptr, wfile_path.c_str(), 0,
					D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, true,
					&d3d_tex_res, &d3d_tex_srv))
				)
			{
				d3d_tex_ = MakeCOMPtr(d3d_tex_res);
				d3d_srv_ = MakeCOMPtr(d3d_tex_srv);
			}
		}
	}

	void SkyBoxRenderable::Render(ID3DX11Effect* effect, ID3DX11EffectPass* pass)
	{
		Matrix rot_view = Renderer::Instance().GetCamera()->view_;
		rot_view.At(3, 0) = 0;
		rot_view.At(3, 1) = 0;
		rot_view.At(3, 2) = 0;
		Matrix proj = Renderer::Instance().GetCamera()->proj_;
		Matrix inv_mvp = Inverse(rot_view * proj);

		SetEffectVar(effect, "g_skybox_tex", d3d_srv_.get());
		SetEffectVar(effect, "g_inv_mvp_mat", inv_mvp);

		QuadRenderable::Render(effect, pass);
	}

}