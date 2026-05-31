#pragma once
#include "FlyingCamera.h"
#include "../JamEngine/RenderGraph.h"

class ComputeScene : public Scene
{
    JAM_SCENE_BODY(ComputeScene);

    struct Demo
    {
        const char* name                        = nullptr;
        void (ComputeScene::*InitFunc)()        = nullptr;
        void (ComputeScene::*ImGuiFuncOrNull)() = nullptr;
    };

    struct Particle
    {
        Vec3  position;
        float lifeTime;
        Vec3  velocity;
        RGBA  color;
    };

public:
    ComputeScene();

    void OnUpdate(float _deltaSec) override;
    void OnRender(const RenderView& _view) override;
    void OnImGuiRender() override;

private:
    void OnResize(Int32 _width, Int32 _height) override;
    void ImGuiRenderOnDemoSelect_();
    void ImGuiRenderOnDemoRun_();
    void RunSelectedDemo_();

    void Demo_ImGui_OnlyComputeButton_();
    void Demo_ImGui_OnlyCaptureButton_();

    void InitDemo1_HelloCompute_();
    void Demo1_Executor_();

    void InitDemo2_AddConstant_();
    void Demo2_Executor_();
    void Demo2_ImGui_();

    void InitDemo3_AddTwoBuffers_();
    void Demo3_Executor_();

    void InitDemo4_Flatten_();
    void Demo4_Executor_();
    void Demo4_ImGui_();

    void InitDemo5_Gradiate_();
    void Demo5_Executor_() const;
    void Demo5_ImGui_();

    void InitDemo6_GrayScale_();
    void Demo6_Executor_();
    void Demo6_ImGui_();

    void InitDemo7_ThreadGroupVisualizeShader_();
    void Demo7_Executor_() const;

    void InitDemo8_BoundsCheck_();
    void Demo8_Executor_();

    void InitDemo9_BlockSum_();
    void Demo9_Executor_();

    void InitDemo10_InterlockedAdd_();
    void Demo10_Executor_();
    void Demo10_ImGui_();

    void InitDemo11_Negative_();
    void Demo11_Executor_();
    void Demo11_ImGui_();

    void InitDemo12_Thresholding_();
    void Demo12_Executor_();
    void Demo12_ImGui_();

    void InitDemo13_1DBoxBlur_();
    void Demo13_Executor_();
    void Demo13_ImGui_();

    void InitDemo14_FindMaximum_();
    void Demo14_Executor_();

    void InitDemo15_ParalleReduction_();
    void Demo15_Executor1_();
    void Demo15_Executor2_();

    void InitDemo16_GaussianBlur_();
    void Demo16_Executor_();
    void Demo16_ImGui_();
    void Demo16_UpdateGaussianFilter_();

    void InitDemo17_GrayScaleHistogram_();
    void Demo17_Executor_();
    void Demo17_ImGui_();


    void ExportTextureWithDialog_(StringView _name) const;

    RenderGraph m_graph        = {};
    Int32       m_selectedDemo = 0;
    bool        m_bDemoRunning = false;

    // storage
    bool   m_bBool0     = false;
    bool   m_bBool1     = false;
    Int32  m_int0       = 0;
    Int32  m_int1       = 0;
    Int32  m_int2       = 0;
    UInt32 m_uint0      = 0;
    UInt32 m_uint1      = 0;
    float  m_float0     = 0.f;
    RGBA   m_rgba0      = RGBA::kBlack;
    RGBA   m_rgba1      = RGBA::kBlack;
    RGBA   m_rgba2      = RGBA::kBlack;
    RGBA   m_rgba3      = RGBA::kBlack;
    Vec3   m_vec30      = Vec3::kZero;
    Vec3   m_vec31      = Vec3::kZero;
    Vec3   m_vec32      = Vec3::kZero;
    Vec3   m_vec33      = Vec3::kZero;
    float  m_kernel[16] = {};

    constexpr static Int32 kNumMaxParticles = 2048;
    Int32                  m_numParticles   = 256;

    FlyingCamera m_camera = {};

    inline static const Demo s_demos[17] = {
        {                 "Demo 1 - Hello Compute",               &ComputeScene::InitDemo1_HelloCompute_, &ComputeScene::Demo_ImGui_OnlyComputeButton_ },
        {                  "Demo 2 - Add Constant",                &ComputeScene::InitDemo2_AddConstant_,                  &ComputeScene::Demo2_ImGui_ },
        {               "Demo 3 - Add Two Buffers",              &ComputeScene::InitDemo3_AddTwoBuffers_, &ComputeScene::Demo_ImGui_OnlyComputeButton_ },
        {                       "Demo 4 - Flatten",                    &ComputeScene::InitDemo4_Flatten_,                  &ComputeScene::Demo4_ImGui_ },
        {                      "Demo 5 - Gradiate",                   &ComputeScene::InitDemo5_Gradiate_,                  &ComputeScene::Demo5_ImGui_ },
        {                    "Demo 6 - Gray Scale",                  &ComputeScene::InitDemo6_GrayScale_,                  &ComputeScene::Demo6_ImGui_ },
        { "Demo 7 - Thread Group Visualize Shader", &ComputeScene::InitDemo7_ThreadGroupVisualizeShader_, &ComputeScene::Demo_ImGui_OnlyCaptureButton_ },
        {                  "Demo 8 - Bounds Check",                &ComputeScene::InitDemo8_BoundsCheck_, &ComputeScene::Demo_ImGui_OnlyComputeButton_ },
        {                     "Demo 9 - Block Sum",                   &ComputeScene::InitDemo9_BlockSum_, &ComputeScene::Demo_ImGui_OnlyComputeButton_ },
        {              "Demo 10 - Interlocked Add",            &ComputeScene::InitDemo10_InterlockedAdd_,                 &ComputeScene::Demo10_ImGui_ },
        {                     "Demo 11 - Nagative",                  &ComputeScene::InitDemo11_Negative_,                 &ComputeScene::Demo11_ImGui_ },
        {                 "Demo 12 - Thresholding",              &ComputeScene::InitDemo12_Thresholding_,                 &ComputeScene::Demo12_ImGui_ },
        {                  "Demo 13 - 1D Box Blur",                 &ComputeScene::InitDemo13_1DBoxBlur_,                 &ComputeScene::Demo13_ImGui_ },
        {                 "Demo 14 - Find Maximum",               &ComputeScene::InitDemo14_FindMaximum_, &ComputeScene::Demo_ImGui_OnlyComputeButton_ },
        {            "Demo 15 - Paralle Reduction",          &ComputeScene::InitDemo15_ParalleReduction_, &ComputeScene::Demo_ImGui_OnlyComputeButton_ },
        {                "Demo 16 - Gaussian Blur",              &ComputeScene::InitDemo16_GaussianBlur_,                 &ComputeScene::Demo16_ImGui_ },
        {         "Demo 17 - Gray Scale Histogram",        &ComputeScene::InitDemo17_GrayScaleHistogram_,                 &ComputeScene::Demo17_ImGui_ },
    };
};
