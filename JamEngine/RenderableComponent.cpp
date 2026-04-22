#include "pch.h"

#include "RenderableComponent.h"

#include "EditorUtilities.h"
#include "EditorWidgets.h"

namespace Jam
{

void RenderableComponent::Serialize(
    ArchiveWriter& _writer,
    Scene*,
    Entity) const
{
    _writer.WriteField("mesh", pMesh);
    _writer.WriteField("material", pMaterial);
}

void RenderableComponent::Deserialize(
    ArchiveReader& _reader,
    Scene*,
    Entity)
{
    _reader.ReadField("mesh", pMesh);
    _reader.ReadField("material", pMaterial);
}

void RenderableComponent::DrawEditPanel(
    Scene*,
    Entity)
{
    ImGui::SeparatorText("mesh");
    if (BeginLabelValueTable("RenderableComponent"))
    {
        // Mesh
        ImGui::TableNextColumn();
        Widget_Text("mesh", true);
        ImGui::TableNextColumn();
        Widget_MeshPicker("##MeshSelector", pMesh);

        ImGui::EndTable();
    }

    ImGui::SeparatorText("material");
    if (BeginLabelValueTable("RenderableComponent"))
    {
        // Material
        ImGui::TableNextColumn();
        Widget_Text("material", true);
        ImGui::TableNextColumn();
        Widget_MaterialPicker("##MaterialSelector", pMaterial);

        //// todo indent?

        //// Material Properties
        //if (pMaterial)
        //{
        //    MaterialDesc             desc     = pMaterial->GetDesc();
        //    MaterialParams&          params   = desc.params;
        //    MaterialTextures&        textures = desc.textures;
        //    MaterialTextureSettings& settings = desc.settings;

        //    bool bDirty = false;

        //    // Phong/Blinn-Phong Properties
        //    {
        //        // Ambient value
        //        ImGui::TableNextColumn();
        //        Widget_Text("ambient value", true);
        //        ImGui::TableNextColumn();
        //        SetNextItemToAvailWidth();
        //        if (ImGui::ColorEdit3("##MeshRendererAmbient", &params.ambient.x))
        //        {
        //            pMaterial->SetDesc(desc);
        //        }

        //        if (ImGui::IsItemDeactivated())
        //        {
        //            bDirty = true;
        //        }

        //        // Diffuse value
        //        ImGui::TableNextColumn();
        //        Widget_Text("diffuse value", true);
        //        ImGui::TableNextColumn();
        //        SetNextItemToAvailWidth();
        //        if (ImGui::ColorEdit3("##MeshRendererDiffuse", &params.diffuse.x))
        //        {
        //            pMaterial->SetDesc(desc);
        //        }

        //        if (ImGui::IsItemDeactivated())
        //        {
        //            bDirty = true;
        //        }

        //        // Specular value
        //        ImGui::TableNextColumn();
        //        Widget_Text("specular value", true);
        //        ImGui::TableNextColumn();
        //        SetNextItemToAvailWidth();
        //        if (ImGui::ColorEdit3("##MeshRendererSpecular", &params.specular.x))
        //        {
        //            pMaterial->SetDesc(desc);
        //        }

        //        if (ImGui::IsItemDeactivated())
        //        {
        //            bDirty = true;
        //        }

        //        // Shininess
        //        ImGui::TableNextColumn();
        //        Widget_Text("shininess", true);
        //        ImGui::TableNextColumn();
        //        SetNextItemToAvailWidth();
        //        if (Widget_DragFloat("##MeshRendererShininess", params.shininess, 0.01f, 0.f, Limits<float>::kMax))
        //        {
        //            pMaterial->SetDesc(desc);
        //        }

        //        if (ImGui::IsItemDeactivated())
        //        {
        //            bDirty = true;
        //        }
        //    }

        //    // PBR Properties
        //    {
        //        // Albedo value
        //        ImGui::TableNextColumn();
        //        Widget_Text("albedo value", true);
        //        ImGui::TableNextColumn();
        //        SetNextItemToAvailWidth();
        //        if (ImGui::ColorEdit3("##MeshRendererAlbedo", &params.albedo.x))
        //        {
        //            pMaterial->SetDesc(desc);
        //        }

        //        if (ImGui::IsItemDeactivated())
        //        {
        //            bDirty = true;
        //        }

        //        // Roughness
        //        ImGui::TableNextColumn();
        //        Widget_Text("roughness", true);
        //        ImGui::TableNextColumn();
        //        SetNextItemToAvailWidth();
        //        if (Widget_DragFloat("##MeshRendererRoughness", params.roughness, 0.01f, 0.f, 1.f))
        //        {
        //            pMaterial->SetDesc(desc);
        //        }

        //        if (ImGui::IsItemDeactivated())
        //        {
        //            bDirty = true;
        //        }

        //        // Metallic
        //        ImGui::TableNextColumn();
        //        Widget_Text("metallic", true);
        //        ImGui::TableNextColumn();
        //        SetNextItemToAvailWidth();
        //        if (Widget_DragFloat("##MeshRendererMetallic", params.metallic, 0.01f, 0.f, 1.f))
        //        {
        //            pMaterial->SetDesc(desc);
        //        }

        //        if (ImGui::IsItemDeactivated())
        //        {
        //            bDirty = true;
        //        }
        //    }

        //    // Emissive Properties
        //    {
        //        // Emissive value
        //        ImGui::TableNextColumn();
        //        Widget_Text("emissive value", true);
        //        ImGui::TableNextColumn();
        //        SetNextItemToAvailWidth();
        //        if (ImGui::ColorEdit3("##MeshRendererEmissive", &params.emissive.x))
        //        {
        //            pMaterial->SetDesc(desc);
        //        }

        //        if (ImGui::IsItemDeactivated())
        //        {
        //            bDirty = true;
        //        }

        //        // Emissive intensity
        //        ImGui::TableNextColumn();
        //        Widget_Text("emissive intensity", true);
        //        ImGui::TableNextColumn();
        //        SetNextItemToAvailWidth();
        //        if (Widget_DragFloat("##MeshRendererEmissiveIntensity", params.emissiveIntensity, 0.01f, 0.f, Limits<float>::kMax))
        //        {
        //            pMaterial->SetDesc(desc);
        //        }

        //        if (ImGui::IsItemDeactivated())
        //        {
        //            bDirty = true;
        //        }
        //    }

        //    // Rendering Options
        //    {
        //        // Alpha cut-off
        //        ImGui::TableNextColumn();
        //        Widget_Text("alpha cut-out", true);
        //        ImGui::TableNextColumn();
        //        SetNextItemToAvailWidth();
        //        if (ImGui::Checkbox("##MeshRendererAlphaCutOff", &params.bAlphaCutout))
        //        {
        //            pMaterial->SetDesc(desc);
        //            bDirty = true;
        //        }

        //        // Double side
        //        ImGui::TableNextColumn();
        //        Widget_Text("double sided", true);
        //        ImGui::TableNextColumn();
        //        SetNextItemToAvailWidth();
        //        if (ImGui::Checkbox("##MeshRendererDoubleSide", &params.bDoubleSided))
        //        {
        //            pMaterial->SetDesc(desc);
        //            bDirty = true;
        //        }
        //    }

        //    // Texture texes
        //    {
        //        // Albedo texture
        //        ImGui::TableNextColumn();
        //        Widget_Text("albedo texture", true);
        //        ImGui::TableNextColumn();
        //        {
        //            Ref<Texture> pTexture = textures[eMaterialTexture::Albedo];
        //            if (Widget_TexturePicker("##MeshRendererAlbedoMap", pTexture))
        //            {
        //                textures[eMaterialTexture::Albedo] = pTexture;
        //                pMaterial->SetDesc(desc);
        //                bDirty = true;
        //            }
        //        }

        //        // Normal texture
        //        ImGui::TableNextColumn();
        //        Widget_Text("normal texture", true);
        //        ImGui::TableNextColumn();
        //        {
        //            Ref<Texture> pTexture = textures[eMaterialTexture::Normal];
        //            if (Widget_TexturePicker("##MeshRendererNormalMap", pTexture))
        //            {
        //                textures[eMaterialTexture::Normal] = pTexture;
        //                pMaterial->SetDesc(desc);
        //                bDirty = true;
        //            }
        //        }

        //        // Flip Y normal texture
        //        ImGui::TableNextColumn();
        //        Widget_Text("texture flip Y", true);
        //        ImGui::TableNextColumn();
        //        if (ImGui::Checkbox("##MeshRendererFlipYNormalMap", &settings.bNormalMapFlipedY))
        //        {
        //            pMaterial->SetDesc(desc);
        //            bDirty = true;
        //        }

        //        // Metallic texture

        //        ImGui::TableNextColumn();
        //        Widget_Text("metallic texture", true);
        //        ImGui::TableNextColumn();
        //        {
        //            Ref<Texture> pTexture = textures[eMaterialTexture::Metallic];
        //            if (Widget_TexturePicker("##MeshRendererMetallicMap", pTexture))
        //            {
        //                textures[eMaterialTexture::Metallic] = pTexture;
        //                pMaterial->SetDesc(desc);
        //                bDirty = true;
        //            }
        //        }

        //        // Metallic texture value channel

        //        ImGui::TableNextColumn();
        //        Widget_Text("texture channel", true);
        //        ImGui::TableNextColumn();
        //        SetNextItemToAvailWidth();
        //        if (Widget_EnumCombo("##MeshRendererMetallicMapColorChannel", settings.metallicMapChannel))
        //        {
        //            pMaterial->SetDesc(desc);
        //            bDirty = true;
        //        }

        //        // Roughness texture

        //        ImGui::TableNextColumn();
        //        Widget_Text("roughness texture", true);
        //        ImGui::TableNextColumn();
        //        {
        //            Ref<Texture> pTexture = textures[eMaterialTexture::Roughness];
        //            if (Widget_TexturePicker("##MeshRendererRoughnessMap", pTexture))
        //            {
        //                textures[eMaterialTexture::Roughness] = pTexture;
        //                pMaterial->SetDesc(desc);
        //                bDirty = true;
        //            }
        //        }

        //        // Roughness texture value channel

        //        ImGui::TableNextColumn();
        //        Widget_Text("texture channel", true);
        //        ImGui::TableNextColumn();
        //        SetNextItemToAvailWidth();
        //        if (Widget_EnumCombo("##MeshRendererRoughnessMapColorChannel", settings.roughnessMapChannel))
        //        {
        //            pMaterial->SetDesc(desc);
        //            bDirty = true;
        //        }

        //        // AO texture
        //        ImGui::TableNextColumn();
        //        Widget_Text("ao texture", true);
        //        ImGui::TableNextColumn();
        //        {
        //            Ref<Texture> pTexture = textures[eMaterialTexture::Ao];
        //            if (Widget_TexturePicker("##MeshRendererAoMap", pTexture))
        //            {
        //                textures[eMaterialTexture::Ao] = pTexture;
        //                pMaterial->SetDesc(desc);
        //                bDirty = true;
        //            }
        //        }

        //        // Emissive texture
        //        ImGui::TableNextColumn();
        //        Widget_Text("emissive texture", true);
        //        ImGui::TableNextColumn();
        //        {
        //            Ref<Texture> pTexture = textures[eMaterialTexture::Emissive];
        //            if (Widget_TexturePicker("##MeshRendererEmissiveMap", pTexture))
        //            {
        //                textures[eMaterialTexture::Emissive] = pTexture;
        //                pMaterial->SetDesc(desc);
        //                bDirty = true;
        //            }
        //        }

        //        // Emissive texture intensity
        //        ImGui::TableNextColumn();
        //        Widget_Text("texture intensity", true);
        //        ImGui::TableNextColumn();
        //        SetNextItemToAvailWidth();
        //        if (Widget_DragFloat("##MeshRendererEmissiveTextureIntensity", settings.emissiveMapIntensity, 0.01f, 0.f, Limits<float>::kMax))
        //        {
        //            pMaterial->SetDesc(desc);
        //        }

        //        if (ImGui::IsItemDeactivated())
        //        {
        //            bDirty = true;
        //        }
        //    }

        //    // Light texture
        //    {
        //        // Light texture
        //        ImGui::TableNextColumn();
        //        Widget_Text("light texture", true);
        //        ImGui::TableNextColumn();
        //        {
        //            Ref<Texture> pTexture = textures[eMaterialTexture::Light];
        //            if (Widget_TexturePicker("##MeshRendererLightMap", pTexture))
        //            {
        //                textures[eMaterialTexture::Light] = pTexture;
        //                pMaterial->SetDesc(desc);
        //                bDirty = true;
        //            }
        //        }

        //        // Light texture intensity
        //        ImGui::TableNextColumn();
        //        Widget_Text("texture intensity", true);
        //        ImGui::TableNextColumn();
        //        SetNextItemToAvailWidth();
        //        if (Widget_DragFloat("##MeshRendererLightMapIntensity", settings.lightMapIntensity, 0.01f, 0.f, Limits<float>::kMax))
        //        {
        //            pMaterial->SetDesc(desc);
        //        }

        //        if (ImGui::IsItemDeactivated())
        //        {
        //            bDirty = true;
        //        }
        //    }

        //    // Vertex Displacement
        //    {
        //        // Displacement texture
        //        ImGui::TableNextColumn();
        //        Widget_Text("displacement texture", true);
        //        ImGui::TableNextColumn();
        //        {
        //            Ref<Texture> pTexture = textures[eMaterialTexture::VertexDisplacement];
        //            if (Widget_TexturePicker("##MeshRendererDisplacementMap", pTexture))
        //            {
        //                textures[eMaterialTexture::VertexDisplacement] = pTexture;
        //                pMaterial->SetDesc(desc);
        //                bDirty = true;
        //            }
        //        }

        //        // Displacement texture scale

        //        ImGui::TableNextColumn();
        //        Widget_Text("texture scale", true);
        //        ImGui::TableNextColumn();
        //        SetNextItemToAvailWidth();
        //        if (Widget_DragFloat("##MeshRendererDisplacementMapScale", settings.vertexDisplacementMapScale, 0.001f, 0.f, Limits<float>::kMax, "%.4f"))
        //        {
        //            pMaterial->SetDesc(desc);
        //        }

        //        if (ImGui::IsItemDeactivated())
        //        {
        //            bDirty = true;
        //        }
        //    }

        //    // Save material if dirty
        //    if (bDirty)
        //    {
        //        StringView path = pMaterial->GetPath();
        //        if (const Error err = pMaterial->Save(path))
        //        {
        //            JAM_LOG_WARN("Failed to save material '{}': {}", path, err.ToErrorMessage());
        //        }
        //    }
        //}

        ImGui::EndTable();
    }
}

}   // namespace Jam