#include "pch.h"

#include "RenderableComponent.h"

#include "EditorUtilities.h"
#include "EditorWidgets.h"

namespace Jam
{

void RenderableComponent::Serialize(
    ArchiveWriter& _writer,
    const Ref<Scene>&,
    Entity) const
{
    _writer.WriteField("mesh", pMesh);
    _writer.WriteField("material", pMaterial);
}

void RenderableComponent::Deserialize(
    ArchiveReader& _reader,
    const Ref<Scene>&,
    Entity)
{
    _reader.ReadField("mesh", pMesh);
    _reader.ReadField("material", pMaterial);
}

void RenderableComponent::DrawEditPanel(
    const Ref<Scene>&,
    Entity)
{
    ImGui::SeparatorText("mesh");
    if (BeginLabelTable("RenderableComponent"))
    {
        // Mesh
        ImGui::TableNextColumn();
        Widget_Text("mesh", true);
        ImGui::TableNextColumn();
        Widget_MeshPicker("##MeshSelector", pMesh);

        ImGui::EndTable();
    }

    ImGui::SeparatorText("material");
    if (BeginLabelTable("RenderableComponent"))
    {
        // Material
        ImGui::TableNextColumn();
        Widget_Text("material", true);
        ImGui::TableNextColumn();
        Widget_MaterialPicker("##MaterialSelector", pMaterial);

        // todo indent?

        // Material Properties
        if (pMaterial)
        {
            MaterialParams          params   = pMaterial->GetParams();
            MaterialTextures        texs     = pMaterial->GetTextures();
            MaterialTextureSettings settings = pMaterial->GetTextureSettings();

            bool bDirty = false;

            // Phong/Blinn-Phong Properties
            {
                // Ambient value
                ImGui::TableNextColumn();
                Widget_Text("ambient value", true);
                ImGui::TableNextColumn();
                SetNextItemToAvailWidth();
                if (ImGui::ColorEdit3("##MeshRendererAmbient", &params.ambient.x))
                {
                    pMaterial->SetParams(params);
                }

                if (ImGui::IsItemDeactivated())
                {
                    bDirty = true;
                }

                // Diffuse value
                ImGui::TableNextColumn();
                Widget_Text("diffuse value", true);
                ImGui::TableNextColumn();
                SetNextItemToAvailWidth();
                if (ImGui::ColorEdit3("##MeshRendererDiffuse", &params.diffuse.x))
                {
                    pMaterial->SetParams(params);
                }

                if (ImGui::IsItemDeactivated())
                {
                    bDirty = true;
                }

                // Specular value
                ImGui::TableNextColumn();
                Widget_Text("specular value", true);
                ImGui::TableNextColumn();
                SetNextItemToAvailWidth();
                if (ImGui::ColorEdit3("##MeshRendererSpecular", &params.specular.x))
                {
                    pMaterial->SetParams(params);
                }

                if (ImGui::IsItemDeactivated())
                {
                    bDirty = true;
                }

                // Shininess
                ImGui::TableNextColumn();
                Widget_Text("shininess", true);
                ImGui::TableNextColumn();
                SetNextItemToAvailWidth();
                if (Widget_DragFloat("##MeshRendererShininess", params.shininess, 0.01f, 0.f, Limits<float>::k_max))
                {
                    pMaterial->SetParams(params);
                }

                if (ImGui::IsItemDeactivated())
                {
                    bDirty = true;
                }
            }

            // PBR Properties
            {
                // Albedo value
                ImGui::TableNextColumn();
                Widget_Text("albedo value", true);
                ImGui::TableNextColumn();
                SetNextItemToAvailWidth();
                if (ImGui::ColorEdit3("##MeshRendererAlbedo", &params.albedo.x))
                {
                    pMaterial->SetParams(params);
                }

                if (ImGui::IsItemDeactivated())
                {
                    bDirty = true;
                }

                // Roughness
                ImGui::TableNextColumn();
                Widget_Text("roughness", true);
                ImGui::TableNextColumn();
                SetNextItemToAvailWidth();
                if (Widget_DragFloat("##MeshRendererRoughness", params.roughness, 0.01f, 0.f, 1.f))
                {
                    pMaterial->SetParams(params);
                }

                if (ImGui::IsItemDeactivated())
                {
                    bDirty = true;
                }

                // Metallic

                ImGui::TableNextColumn();
                Widget_Text("metallic", true);
                ImGui::TableNextColumn();
                SetNextItemToAvailWidth();
                if (Widget_DragFloat("##MeshRendererMetallic", params.metallic, 0.01f, 0.f, 1.f))
                {
                    pMaterial->SetParams(params);
                }

                if (ImGui::IsItemDeactivated())
                {
                    bDirty = true;
                }
            }

            // Emissive Properties
            {
                // Emissive value

                ImGui::TableNextColumn();
                Widget_Text("emissive value", true);
                ImGui::TableNextColumn();
                SetNextItemToAvailWidth();
                if (ImGui::ColorEdit3("##MeshRendererEmissive", &params.emissive.x))
                {
                    pMaterial->SetParams(params);
                }

                if (ImGui::IsItemDeactivated())
                {
                    bDirty = true;
                }

                // Emissive intensity

                ImGui::TableNextColumn();
                Widget_Text("emissive intensity", true);
                ImGui::TableNextColumn();
                SetNextItemToAvailWidth();
                if (Widget_DragFloat("##MeshRendererEmissiveIntensity", params.emissiveIntensity, 0.01f, 0.f, Limits<float>::k_max))
                {
                    pMaterial->SetParams(params);
                }

                if (ImGui::IsItemDeactivated())
                {
                    bDirty = true;
                }
            }

            // Rendering Options
            {
                // Alpha cut-off
                ImGui::TableNextColumn();
                Widget_Text("alpha cut-out", true);
                ImGui::TableNextColumn();
                SetNextItemToAvailWidth();
                if (ImGui::Checkbox("##MeshRendererAlphaCutOff", &params.bAlphaCutout))
                {
                    pMaterial->SetParams(params);
                    bDirty = true;
                }

                // Double side

                ImGui::TableNextColumn();
                Widget_Text("double sided", true);
                ImGui::TableNextColumn();
                SetNextItemToAvailWidth();
                if (ImGui::Checkbox("##MeshRendererDoubleSide", &params.bDoubleSided))
                {
                    pMaterial->SetParams(params);
                    bDirty = true;
                }
            }

            // Texture texes
            {
                // Albedo tex

                ImGui::TableNextColumn();
                Widget_Text("albedo texture", true);
                ImGui::TableNextColumn();
                {
                    Ref<Texture> tex = texs.Get(eMaterialTextureType::Albedo);
                    if (Widget_TexturePicker("##MeshRendererAlbedoMap", tex))
                    {
                        pMaterial->SetTexture(eMaterialTextureType::Albedo, tex);
                        bDirty = true;
                    }
                }

                // Normal tex

                ImGui::TableNextColumn();
                Widget_Text("normal texture", true);
                ImGui::TableNextColumn();
                {
                    Ref<Texture> tex = texs.Get(eMaterialTextureType::Normal);
                    if (Widget_TexturePicker("##MeshRendererNormalMap", tex))
                    {
                        pMaterial->SetTexture(eMaterialTextureType::Normal, tex);
                        bDirty = true;
                    }
                }

                // Flip Y normal tex

                ImGui::TableNextColumn();
                Widget_Text("texture flip Y", true);
                ImGui::TableNextColumn();
                if (ImGui::Checkbox("##MeshRendererFlipYNormalMap", &settings.bNormalMapFlipedY))
                {
                    pMaterial->SetTextureSettings(settings);
                    bDirty = true;
                }

                // Metallic tex

                ImGui::TableNextColumn();
                Widget_Text("metallic texture", true);
                ImGui::TableNextColumn();
                {
                    Ref<Texture> tex = texs.Get(eMaterialTextureType::Metallic);
                    if (Widget_TexturePicker("##MeshRendererMetallicMap", tex))
                    {
                        pMaterial->SetTexture(eMaterialTextureType::Metallic, tex);
                        bDirty = true;
                    }
                }

                // Metallic tex value channel

                ImGui::TableNextColumn();
                Widget_Text("texture channel", true);
                ImGui::TableNextColumn();
                SetNextItemToAvailWidth();
                if (Widget_EnumCombo("##MeshRendererMetallicMapColorChannel", settings.metallicMapChannel))
                {
                    pMaterial->SetTextureSettings(settings);
                    bDirty = true;
                }

                // Roughness tex

                ImGui::TableNextColumn();
                Widget_Text("roughness texture", true);
                ImGui::TableNextColumn();
                {
                    Ref<Texture> tex = texs.Get(eMaterialTextureType::Roughness);
                    if (Widget_TexturePicker("##MeshRendererRoughnessMap", tex))
                    {
                        pMaterial->SetTexture(eMaterialTextureType::Roughness, tex);
                        bDirty = true;
                    }
                }

                // Roughness tex value channel

                ImGui::TableNextColumn();
                Widget_Text("texture channel", true);
                ImGui::TableNextColumn();
                SetNextItemToAvailWidth();
                if (Widget_EnumCombo("##MeshRendererRoughnessMapColorChannel", settings.roughnessMapChannel))
                {
                    pMaterial->SetTextureSettings(settings);
                    bDirty = true;
                }

                // AO tex
                ImGui::TableNextColumn();
                Widget_Text("ao texture", true);
                ImGui::TableNextColumn();
                {
                    Ref<Texture> tex = texs.Get(eMaterialTextureType::Ao);
                    if (Widget_TexturePicker("##MeshRendererAoMap", tex))
                    {
                        pMaterial->SetTexture(eMaterialTextureType::Ao, tex);
                        bDirty = true;
                    }
                }

                // Emissive tex
                ImGui::TableNextColumn();
                Widget_Text("emissive texture", true);
                ImGui::TableNextColumn();
                {
                    Ref<Texture> tex = texs.Get(eMaterialTextureType::Emissive);
                    if (Widget_TexturePicker("##MeshRendererEmissiveMap", tex))
                    {
                        pMaterial->SetTexture(eMaterialTextureType::Emissive, tex);
                        bDirty = true;
                    }
                }

                // Emissive tex intensity
                ImGui::TableNextColumn();
                Widget_Text("texture intensity", true);
                ImGui::TableNextColumn();
                SetNextItemToAvailWidth();
                if (Widget_DragFloat("##MeshRendererEmissiveTextureIntensity", settings.emissiveMapIntensity, 0.01f, 0.f, Limits<float>::k_max))
                {
                    pMaterial->SetTextureSettings(settings);
                }

                if (ImGui::IsItemDeactivated())
                {
                    bDirty = true;
                }
            }

            // Light tex
            {
                // Light tex
                ImGui::TableNextColumn();
                Widget_Text("light texture", true);
                ImGui::TableNextColumn();
                {
                    Ref<Texture> tex = texs.Get(eMaterialTextureType::Light);
                    if (Widget_TexturePicker("##MeshRendererLightMap", tex))
                    {
                        pMaterial->SetTexture(eMaterialTextureType::Light, tex);
                        bDirty = true;
                    }
                }

                // Light tex intensity
                ImGui::TableNextColumn();
                Widget_Text("texture intensity", true);
                ImGui::TableNextColumn();
                SetNextItemToAvailWidth();
                if (Widget_DragFloat("##MeshRendererLightMapIntensity", settings.lightMapIntensity, 0.01f, 0.f, Limits<float>::k_max))
                {
                    pMaterial->SetTextureSettings(settings);
                }

                if (ImGui::IsItemDeactivated())
                {
                    bDirty = true;
                }
            }

            // Vertex Displacement
            {
                // Displacement tex
                ImGui::TableNextColumn();
                Widget_Text("displacement texture", true);
                ImGui::TableNextColumn();
                {
                    Ref<Texture> tex = texs.Get(eMaterialTextureType::VertexDisplacement);
                    if (Widget_TexturePicker("##MeshRendererDisplacementMap", tex))
                    {
                        pMaterial->SetTexture(eMaterialTextureType::VertexDisplacement, tex);
                        bDirty = true;
                    }
                }

                // Displacement tex scale

                ImGui::TableNextColumn();
                Widget_Text("texture scale", true);
                ImGui::TableNextColumn();
                SetNextItemToAvailWidth();
                if (Widget_DragFloat("##MeshRendererDisplacementMapScale", settings.vertexDisplacementMapScale, 0.001f, 0.f, Limits<float>::k_max, "%.4f"))
                {
                    pMaterial->SetTextureSettings(settings);
                }

                if (ImGui::IsItemDeactivated())
                {
                    bDirty = true;
                }
            }

            // Save material if dirty
            if (bDirty)
            {
                const Path& path = pMaterial->GetPath();
                if (const Error err = pMaterial->Save(path))
                {
                    JAM_LOG.Warn("Failed to save material '{}': {}", path, err.ToErrorMessage());
                }
            }
        }

        ImGui::EndTable();
    }
}

}   // namespace Jam