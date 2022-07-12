// This file was developed by Thomas Müller <thomas94@gmx.net>.
// It is published under the BSD 3-Clause License within the LICENSE file.
//{{{  includes
#include <FalseColor.h>
#include <UberShader.h>

using namespace nanogui;
using namespace std;
//}}}
TEV_NAMESPACE_BEGIN

//{{{
UberShader::UberShader (RenderPass* renderPass) {

  try {
    #if defined(NANOGUI_USE_OPENGL)
      std::string preamble = R"(#version 110)";
    #elif defined(NANOGUI_USE_GLES)
      std::string preamble =
        R"(#version 100
           precision highp float;)";
    #endif
    //{{{
    auto vertexShader = preamble +
        R"(
        uniform vec2 pixelSize;
        uniform vec2 checkerSize;

        uniform vec2 imageScale;
        uniform vec2 imageOffset;

        uniform vec2 referenceScale;
        uniform vec2 referenceOffset;

        attribute vec2 position;

        varying vec2 checkerUv;
        varying vec2 imageUv;
        varying vec2 referenceUv;

        void main() {
          checkerUv = position / (pixelSize * checkerSize);
          imageUv = position * imageScale + imageOffset;
          referenceUv = position * referenceScale + referenceOffset;

          gl_Position = vec4(position, 1.0, 1.0);
        })";
    //}}}
    //{{{
    auto fragmentShader = preamble +
        R"(
        #define SRGB        0
        #define GAMMA       1
        #define FALSE_COLOR 2
        #define POS_NEG     3

        #define ERROR                   0
        #define ABSOLUTE_ERROR          1
        #define SQUARED_ERROR           2
        #define RELATIVE_ABSOLUTE_ERROR 3
        #define RELATIVE_SQUARED_ERROR  4

        uniform sampler2D image;
        uniform bool hasImage;

        uniform sampler2D reference;
        uniform bool hasReference;

        uniform sampler2D colormap;

        uniform float exposure;
        uniform float offset;
        uniform float gamma;
        uniform bool clipToLdr;
        uniform int tonemap;
        uniform int metric;

        uniform vec4 bgColor;

        varying vec2 checkerUv;
        varying vec2 imageUv;
        varying vec2 referenceUv;

        float average(vec3 col) {
          return (col.r + col.g + col.b) / 3.0;
          }

        vec3 applyExposureAndOffset(vec3 col) {
          return pow(2.0, exposure) * col + offset;
          }

        vec3 falseColor(float v) {
          v = clamp(v, 0.0, 1.0);
          return texture2D(colormap, vec2(v, 0.5)).rgb;
          }

        float linear(float sRGB) {
          float outSign = sign(sRGB);
          sRGB = abs(sRGB);
          if (sRGB <= 0.04045) {
            return outSign * sRGB / 12.92;
            }
          else {
            return outSign * pow((sRGB + 0.055) / 1.055, 2.4);
            }
          }

        float sRGB(float linear) {
          float outSign = sign(linear);
          linear = abs(linear);
          if (linear < 0.0031308) {
            return outSign * 12.92 * linear;
            }
          else {
            return outSign * 1.055 * pow(linear, 0.41666) - 0.055;
            }
          }

        vec3 applyTonemap(vec3 col, vec4 background) {
            if (tonemap == SRGB) {
                col = col + (vec3(linear(background.r), linear(background.g), linear(background.b)) - offset) * background.a;
                return vec3(sRGB(col.r), sRGB(col.g), sRGB(col.b));
            } else if (tonemap == GAMMA) {
                col = col + (pow(background.rgb, vec3(gamma)) - offset) * background.a;
                return sign(col) * pow(abs(col), vec3(1.0 / gamma));
            } else if (tonemap == FALSE_COLOR) {
                return falseColor(log2(average(col)+0.03125) / 10.0 + 0.5) + (background.rgb - falseColor(0.0)) * background.a;
            } else if (tonemap == POS_NEG) {
                return vec3(-average(min(col, vec3(0.0))) * 2.0, average(max(col, vec3(0.0))) * 2.0, 0.0) + background.rgb * background.a;
            }
            return vec3(0.0);
        }

        vec3 applyMetric(vec3 col, vec3 reference) {
            if (metric == ERROR) {
                return col;
            } else if (metric == ABSOLUTE_ERROR) {
                return abs(col);
            } else if (metric == SQUARED_ERROR) {
                return col * col;
            } else if (metric == RELATIVE_ABSOLUTE_ERROR) {
                return abs(col) / (reference + vec3(0.01));
            } else if (metric == RELATIVE_SQUARED_ERROR) {
                return col * col / (reference * reference + vec3(0.01));
            }
            return vec3(0.0);
        }

        vec4 sample(sampler2D sampler, vec2 uv) {
          vec4 color = texture2D(sampler, uv);
          if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
            color = vec4(0.0);
            }
          return color;
          }

        void main() {
          vec3 darkGray = vec3(0.5, 0.5, 0.5);
          vec3 lightGray = vec3(0.55, 0.55, 0.55);
          vec3 checker = abs(mod(floor(checkerUv.x) + floor(checkerUv.y), 2.0)) < 0.5 ? darkGray : lightGray;
          checker = bgColor.rgb * bgColor.a + checker * (1.0 - bgColor.a);
          if (!hasImage) {
            gl_FragColor = vec4(checker, 1.0);
            return;
            }

          vec4 imageVal = sample(image, imageUv);
          if (!hasReference) {
            gl_FragColor = vec4 (applyTonemap(applyExposureAndOffset(imageVal.rgb), vec4(checker, 1.0 - imageVal.a)), 1.0);
            gl_FragColor.rgb = clamp(gl_FragColor.rgb, clipToLdr ? 0.0 : -64.0, clipToLdr ? 1.0 : 64.0);
            return;
            }

          vec4 referenceVal = sample(reference, referenceUv);
          vec3 difference = imageVal.rgb - referenceVal.rgb;
          float alpha = (imageVal.a + referenceVal.a) * 0.5;
          gl_FragColor = vec4 (applyTonemap(applyExposureAndOffset (
                                 applyMetric(difference, referenceVal.rgb)), vec4(checker, 1.0 - alpha)), 1.0);
          gl_FragColor.rgb = clamp(gl_FragColor.rgb, clipToLdr ? 0.0 : -64.0, clipToLdr ? 1.0 : 64.0);
          }
        )";
    //}}}

    mShader = new Shader {renderPass, "ubershader", vertexShader, fragmentShader};
    }
  catch (const runtime_error& e) {
    tlog::error() << tfm::format ("Unable to compile shader: %s", e.what());
    }

  // 2 Triangles
  uint32_t indices[3*2] = { 0, 1, 2,
                            2, 3, 0, };

  float positions[2*4] = { -1.f, -1.f,
                            1.f, -1.f,
                            1.f,  1.f,
                           -1.f,  1.f, };

  mShader->set_buffer ("indices", VariableType::UInt32, {3*2}, indices);
  mShader->set_buffer ("position", VariableType::Float32, {4, 2}, positions);

  const auto& fcd = colormap::turbo();

  mColorMap = new Texture {Texture::PixelFormat::RGBA, Texture::ComponentFormat::Float32, Vector2i{(int)fcd.size() / 4, 1} };

  mColorMap->upload ((uint8_t*)fcd.data());
  }
//}}}
UberShader::~UberShader() {}

//{{{
void UberShader::draw (const Vector2f& pixelSize, const Vector2f& checkerSize) {

  draw (pixelSize, checkerSize, nullptr, Matrix3f{0.0f}, 0.0f, 0.0f, 0.0f, false, ETonemap::SRGB);
  }
//}}}
//{{{
void UberShader::draw (const Vector2f& pixelSize, const Vector2f& checkerSize, Texture* textureImage,
                       const Matrix3f& transformImage,
                       float exposure, float offset, float gamma,
                       bool clipToLdr, ETonemap tonemap) {

  draw (pixelSize, checkerSize, textureImage, transformImage, nullptr, Matrix3f{0.0f},
        exposure, offset, gamma, clipToLdr, tonemap, EMetric::Error);
  }
//}}}
//{{{
void UberShader::draw (const Vector2f& pixelSize, const Vector2f& checkerSize, Texture* textureImage,
                       const Matrix3f& transformImage,
                       Texture* textureReference,
                       const Matrix3f& transformReference,
                       float exposure, float offset, float gamma,
                       bool clipToLdr, ETonemap tonemap, EMetric metric) {

  bool hasImage = textureImage;
  if (!hasImage) // Just to have _some_ valid texture to bind. Will be ignored.
    textureImage = mColorMap.get();

  bool hasReference = textureReference;
  if (!hasReference) // Just to have _some_ valid texture to bind. Will be ignored.
    textureReference = textureImage;

  bindCheckerboardData (pixelSize, checkerSize);
  bindImageData (textureImage, transformImage, exposure, offset, gamma, tonemap);
  bindReferenceData (textureReference, transformReference, metric);

  mShader->set_uniform ("hasImage", hasImage);
  mShader->set_uniform ("hasReference", hasReference);
  mShader->set_uniform ("clipToLdr", clipToLdr);

  mShader->begin();
  mShader->draw_array (Shader::PrimitiveType::Triangle, 0, 6, true);
  mShader->end();
 }
//}}}

//{{{
void UberShader::bindCheckerboardData (const Vector2f& pixelSize, const Vector2f& checkerSize) {

  mShader->set_uniform ("pixelSize", pixelSize);
  mShader->set_uniform ("checkerSize", checkerSize);
  mShader->set_uniform ("bgColor", mBackgroundColor);
  }
//}}}
//{{{
void UberShader::bindImageData (Texture* textureImage,
                                const Matrix3f& transformImage,
                                float exposure,
                                float offset,
                                float gamma,
                                ETonemap tonemap) {

  mShader->set_texture ("image", textureImage);
  mShader->set_uniform ("imageScale", Vector2f{transformImage.m[0][0], transformImage.m[1][1]});
  mShader->set_uniform ("imageOffset", Vector2f{transformImage.m[2][0], transformImage.m[2][1]});

  mShader->set_uniform ("exposure", exposure);
  mShader->set_uniform ("offset", offset);
  mShader->set_uniform ("gamma", gamma);
  mShader->set_uniform ("tonemap", static_cast<int>(tonemap));

  mShader->set_texture ("colormap", mColorMap.get());
  }
//}}}
//{{{
void UberShader::bindReferenceData (Texture* textureReference,
                                    const Matrix3f& transformReference,
                                    EMetric metric) {

  mShader->set_texture ("reference", textureReference);
  mShader->set_uniform ("referenceScale", Vector2f{transformReference.m[0][0], transformReference.m[1][1]});
  mShader->set_uniform ("referenceOffset", Vector2f{transformReference.m[2][0], transformReference.m[2][1]});

  mShader->set_uniform ("metric", static_cast<int>(metric));
  }
//}}}

TEV_NAMESPACE_END
