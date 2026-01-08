#include "pch.h"

#include "RenderableComponent.h"

#include "EditorUtilities.h"
#include "EditorWidgets.h"
#include "ImGuiUtilities.h"
#include "Material.h"
#include "JsonUtilities.h"

namespace jam
{

void RenderableComponent::Serialize(
    JsonWriter        _writer,
    const Ref<Scene>& _pScene,
    Entity            _owner) const
{
    _writer.WriteField("mesh", renderable.pMesh);
    _writer.WriteField("material", renderable.pMaterial);
}

void RenderableComponent::Deserialize(
    JsonReader        _reader,
    const Ref<Scene>& _pScene,
    Entity            _owner)
{
    _reader.ReadField("mesh", renderable.pMesh);
    _reader.ReadField("material", renderable.pMaterial);
}

void RenderableComponent::Panel_EditComponent(Ref<Scene>& _pScene, Entity _entity)
{
    ImGui::SeparatorText("mesh");
    if (BeginLabelValueTable("MeshRendererComponent"))
    {
        // Mesh
        {
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("mesh");
            ImGui::TableNextColumn();
            Widget_MeshSelector("##MeshSelector", renderable.pMesh);
        }

        ImGui::EndTable()();
    }

    ImGui::SeparatorText("material");
    if (BeginLabelValueTable("MeshRendererMaterial"))
    {

        // Material
        {
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("material");
            ImGui::TableNextColumn();
            Widget_MaterialSelector("##MaterialSelector", renderable.pMaterial);
        }

        // Material Properties
        if (renderable.pMaterial)
        {
            Vec3              ambientCol           = renderable.pMaterial->GetAmbientColor();
            Vec3              diffuseCol           = renderable.pMaterial->GetDiffuseColor();
            Vec3              specularCol          = renderable.pMaterial->GetSpecularColor();
            float             shininess            = renderable.pMaterial->GetShininess();
            Vec3              albedoCol            = renderable.pMaterial->GetAlbedoColor();
            float             roughness            = renderable.pMaterial->GetRoughness();
            float             metallic             = renderable.pMaterial->GetMetallic();
            Vec3              emissiveCol          = renderable.pMaterial->GetEmissiveColor();
            float             emissiveIntensity    = renderable.pMaterial->GetEmissiveIntensity();
            bool              bAlphaCutout         = renderable.pMaterial->IsAlphaCutout();
            bool              bDoubleSide          = renderable.pMaterial->IsDoubleSide();
            bool              bFlipYNormalMap      = renderable.pMaterial->IsFlipYNormalTexture();
            eTextureChannel   metallicChannel      = renderable.pMaterial->GetMetallicTextureChannel();
            eTextureChannel   roughnessChannel     = renderable.pMaterial->GetRoughnessTextureChannel();
            float             emissiveTexIntensity = renderable.pMaterial->GetEmissiveTextureIntensity();
            float             lightTexIntensity    = renderable.pMaterial->GetLightTextureIntensity();
            float             displacementScale    = renderable.pMaterial->GetVertexDisplacementTextureScale();
            Ref<ImageTexture> albedoTex            = renderable.pMaterial->GetAlbedoTexture();
            Ref<ImageTexture> normalTex            = renderable.pMaterial->GetNormalTexture();
            Ref<ImageTexture> metallicTex          = renderable.pMaterial->GetMetallicTexture();
            Ref<ImageTexture> roughnessTex         = renderable.pMaterial->GetRoughnessTexture();
            Ref<ImageTexture> aoTex                = renderable.pMaterial->GetAoTexture();
            Ref<ImageTexture> emissiveTex          = renderable.pMaterial->GetEmissiveTexture();
            Ref<ImageTexture> lightTex             = renderable.pMaterial->GetLightTexture();
            Ref<ImageTexture> displacementTex      = renderable.pMaterial->GetVertexDisplacementTexture();

            bool bDirty = false;

            // Phong/Blinn-Phong Properties
            {
                // Ambient color
                ImGui::TableNextColumn();
                Widget_FrameAlignedText("ambient color");
                ImGui::TableNextColumn();
                SetNextItemAvailWidth();
                if (ImGui::ColorEdit3("##MeshRendererAmbient", &ambientCol.x))
                {
                    renderable.pMaterial->SetAmbientColor(ambientCol);
                }

                if (ImGui::IsItemDeactivated())
                {
                    bDirty = true;
                }

                // Diffuse color
                ImGui::TableNextColumn();
                Widget_FrameAlignedText("diffuse color");
                ImGui::TableNextColumn();
                SetNextItemAvailWidth();
                if (ImGui::ColorEdit3("##MeshRendererDiffuse", &diffuseCol.x))
                {
                    renderable.pMaterial->SetDiffuseColor(diffuseCol);
                }

                if (ImGui::IsItemDeactivated())
                {
                    bDirty = true;
                }

                // Specular color
                ImGui::TableNextColumn();
                Widget_FrameAlignedText("specular color");
                ImGui::TableNextColumn();
                SetNextItemAvailWidth();
                if (ImGui::ColorEdit3("##MeshRendererSpecular", &specularCol.x))
                {
                    renderable.pMaterial->SetSpecularColor(specularCol);
                }

                if (ImGui::IsItemDeactivated())
                {
                    bDirty = true;
                }

                // Shininess
                ImGui::TableNextColumn();
                Widget_FrameAlignedText("shininess");
                ImGui::TableNextColumn();
                SetNextItemAvailWidth();
                if (Widget_DragFloat("##MeshRendererShininess", &shininess, 0.1f, 0.f, 1000.f, "%.1f"))
                {
                    renderable.pMaterial->SetShininess(shininess);
                }

                if (ImGui::IsItemDeactivated())
                {
                    bDirty = true;
                }
            }

            // PBR Properties
            {
                // Albedo color
                ImGui::TableNextColumn();
                Widget_FrameAlignedText("albedo color");
                ImGui::TableNextColumn();
                SetNextItemAvailWidth();
                if (ImGui::ColorEdit3("##MeshRendererAlbedo", &albedoCol.x))
                {
                    renderable.pMaterial->SetAlbedoColor(albedoCol);
                }

                if (ImGui::IsItemDeactivated())
                {
                    bDirty = true;
                }

                // Roughness
                ImGui::TableNextColumn();
                Widget_FrameAlignedText("roughness");
                ImGui::TableNextColumn();
                SetNextItemAvailWidth();
                if (Widget_DragFloat("##MeshRendererRoughness", &roughness, 0.01f, 0.f, 1.f))
                {
                    renderable.pMaterial->SetRoughness(roughness);
                }

                if (ImGui::IsItemDeactivated())
                {
                    bDirty = true;
                }

                // Metallic

                ImGui::TableNextColumn();
                Widget_FrameAlignedText("metallic");
                ImGui::TableNextColumn();
                SetNextItemAvailWidth();
                if (Widget_DragFloat("##MeshRendererMetallic", &metallic, 0.01f, 0.f, 1.f))
                {
                    renderable.pMaterial->SetMetallic(metallic);
                }

                if (ImGui::IsItemDeactivated())
                {
                    bDirty = true;
                }
            }

            // Emissive Properties
            {
                // Emissive color

                ImGui::TableNextColumn();
                Widget_FrameAlignedText("emissive color");
                ImGui::TableNextColumn();
                SetNextItemAvailWidth();
                if (ImGui::ColorEdit3("##MeshRendererEmissive", &emissiveCol.x))
                {
                    renderable.pMaterial->SetEmissiveColor(emissiveCol);
                }

                if (ImGui::IsItemDeactivated())
                {
                    bDirty = true;
                }

                // Emissive intensity

                ImGui::TableNextColumn();
                Widget_FrameAlignedText("emissive intensity");
                ImGui::TableNextColumn();
                SetNextItemAvailWidth();
                if (Widget_DragFloat("##MeshRendererEmissiveIntensity", &emissiveIntensity, 0.01f, 0.f))
                {
                    renderable.pMaterial->SetEmissiveIntensity(emissiveIntensity);
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
                Widget_FrameAlignedText("alpha cut-out");
                ImGui::TableNextColumn();
                SetNextItemAvailWidth();
                if (ImGui::Checkbox("##MeshRendererAlphaCutOff", &bAlphaCutout))
                {
                    renderable.pMaterial->SetAlphaCutout(bAlphaCutout);
                    bDirty = true;
                }

                // Double side

                ImGui::TableNextColumn();
                Widget_FrameAlignedText("double sided");
                ImGui::TableNextColumn();
                SetNextItemAvailWidth();
                if (ImGui::Checkbox("##MeshRendererDoubleSide", &bDoubleSide))
                {
                    renderable.pMaterial->SetDoubleSide(bDoubleSide);
                    bDirty = true;
                }
            }

            // Texture textures
            {
                // Albedo texture

                ImGui::TableNextColumn();
                Widget_FrameAlignedText("albedo texture");
                ImGui::TableNextColumn();
                if (Widget_ImageTextureSelector("##MeshRendererAlbedoMap", albedoTex))
                {
                    renderable.pMaterial->SetAlbedoTexture(albedoTex);
                    bDirty = true;
                }

                // Normal texture

                ImGui::TableNextColumn();
                Widget_FrameAlignedText("normal texture");
                ImGui::TableNextColumn();
                if (Widget_ImageTextureSelector("##MeshRendererNormalMap", normalTex))
                {
                    renderable.pMaterial->SetNormalTexture(normalTex);
                    bDirty = true;
                }

                // Flip Y normal texture

                ImGui::TableNextColumn();
                Widget_FrameAlignedText("texture flip Y");
                ImGui::TableNextColumn();
                if (ImGui::Checkbox("##MeshRendererFlipYNormalMap", &bFlipYNormalMap))
                {
                    renderable.pMaterial->SetFlipYNormalTexture(bFlipYNormalMap);
                    bDirty = true;
                }

                // Metallic texture

                ImGui::TableNextColumn();
                Widget_FrameAlignedText("metallic texture");
                ImGui::TableNextColumn();
                if (Widget_ImageTextureSelector("##MeshRendererMetallicMap", metallicTex))
                {
                    renderable.pMaterial->SetMetallicTexture(metallicTex);
                    bDirty = true;
                }

                // Metallic texture color channel

                ImGui::TableNextColumn();
                Widget_FrameAlignedText("texture channel");
                ImGui::TableNextColumn();
                SetNextItemAvailWidth();
                if (Widget_TextureChannelCombo("##MeshRendererMetallicMapColorChannel", metallicChannel, true))
                {
                    renderable.pMaterial->SetMetallicTextureChannel(metallicChannel);
                    bDirty = true;
                }

                // Roughness texture

                ImGui::TableNextColumn();
                Widget_FrameAlignedText("roughness texture");
                ImGui::TableNextColumn();
                if (Widget_ImageTextureSelector("##MeshRendererRoughnessMap", roughnessTex))
                {
                    renderable.pMaterial->SetRoughnessTexture(roughnessTex);
                    bDirty = true;
                }

                // Roughness texture color channel

                ImGui::TableNextColumn();
                Widget_FrameAlignedText("texture channel");
                ImGui::TableNextColumn();
                SetNextItemAvailWidth();
                if (Widget_TextureChannelCombo("##MeshRendererRoughnessMapColorChannel", roughnessChannel, true))
                {
                    renderable.pMaterial->SetRoughnessTextureChannel(roughnessChannel);
                    bDirty = true;
                }

                // AO texture
                ImGui::TableNextColumn();
                Widget_FrameAlignedText("ao texture");
                ImGui::TableNextColumn();
                if (Widget_ImageTextureSelector("##MeshRendererAoMap", aoTex))
                {
                    renderable.pMaterial->SetAoTexture(aoTex);
                    bDirty = true;
                }

                // Emissive texture
                ImGui::TableNextColumn();
                Widget_FrameAlignedText("emissive texture");
                ImGui::TableNextColumn();
                if (Widget_ImageTextureSelector("##MeshRendererEmissiveMap", emissiveTex))
                {
                    renderable.pMaterial->SetEmissiveTexture(emissiveTex);
                    bDirty = true;
                }

                // Emissive texture intensity
                ImGui::TableNextColumn();
                Widget_FrameAlignedText("texture intensity");
                ImGui::TableNextColumn();
                SetNextItemAvailWidth();
                if (Widget_DragFloat("##MeshRendererEmissiveTextureIntensity", &emissiveTexIntensity, 0.01f, 0.f))
                {
                    renderable.pMaterial->SetEmissiveTextureIntensity(emissiveTexIntensity);
                }

                if (ImGui::IsItemDeactivated())
                {
                    bDirty = true;
                }
            }

            // Light texture
            {
                // Light texture
                ImGui::TableNextColumn();
                Widget_FrameAlignedText("light texture");
                ImGui::TableNextColumn();
                if (Widget_ImageTextureSelector("##MeshRendererLightMap", lightTex))
                {
                    renderable.pMaterial->SetLightTexture(lightTex);
                    bDirty = true;
                }

                // Light texture intensity
                ImGui::TableNextColumn();
                Widget_FrameAlignedText("texture intensity");
                ImGui::TableNextColumn();
                SetNextItemAvailWidth();
                if (Widget_DragFloat("##MeshRendererLightMapIntensity", &lightTexIntensity, 0.01f, 0.f))
                {
                    renderable.pMaterial->SetLightTextureIntensity(lightTexIntensity);
                }

                if (ImGui::IsItemDeactivated())
                {
                    bDirty = true;
                }
            }

            // Vertex Displacement
            {
                // Displacement texture

                ImGui::TableNextColumn();
                Widget_FrameAlignedText("displacement texture");
                ImGui::TableNextColumn();
                if (Widget_ImageTextureSelector("##MeshRendererDisplacementMap", displacementTex))
                {
                    renderable.pMaterial->SetVertexDisplacementTexture(displacementTex);
                    bDirty = true;
                }

                // Displacement texture scale

                ImGui::TableNextColumn();
                Widget_FrameAlignedText("texture scale");
                ImGui::TableNextColumn();
                SetNextItemAvailWidth();
                if (Widget_DragFloat("##MeshRendererDisplacementMapScale", &displacementScale, 0.001f, 0.f, FLT_MAX, "%.4f"))
                {
                    renderable.pMaterial->SetVertexDisplacementTextureScale(displacementScale);
                }

                if (ImGui::IsItemDeactivated())
                {
                    bDirty = true;
                }
            }

            // Save material if dirty
            if (bDirty)
            {
                const fs::path& path = renderable.pMaterial->GetPath();
                renderable.pMaterial->Save(path, TODO);
            }
        }

        ImGui::EndTable()();
    }
}

}   // namespace jam