#pragma once

using byte   = unsigned char;
using uint8  = unsigned char;
using uint16 = unsigned short;
using uint32 = unsigned int;
using uint64 = unsigned long long;

using int8  = char;
using int16 = short;
using int32 = int;
using int64 = long long;

struct VkBuffer_T;
struct VkImage_T;
struct VkImageView_T;
struct VkDeviceMemory_T;

struct VkCommandBuffer_T;

namespace Usage {
    // Vulkan SDK 1.4.304.0
    enum class Buffer: uint32 {
        NONE                                         = 0x00000000U,
        TRANSFER_SRC                                 = 0x00000001U,
        TRANSFER_DST                                 = 0x00000002U,
        UNIFORM_TEXEL                                = 0x00000004U,
        UTORAGE_TEXEL                                = 0x00000008U,
        UNIFORM                                      = 0x00000010U,
        STORAGE                                      = 0x00000020U,
        INDEX                                        = 0x00000040U,
        VERTEX                                       = 0x00000080U,
        INDIRECT                                     = 0x00000100U,
        CONDITIONAL_RENDERING                        = 0x00000200U,     // EXT
        SHADER_BINDING_TABLE                         = 0x00000400U,     // KHR
        RAY_TRACING                                  = 0x00000400U,     // NV
        TRANSFORM_FEEDBACK                           = 0x00000800U,     // EXT
        TRANSFORM_FEEDBACK_COUNTER                   = 0x00001000U,     // EXT
        VIDEO_DECODE_SRC                             = 0x00002000U,     // KHR
        VIDEO_DECODE_DST                             = 0x00004000U,     // KHR
        VIDEO_ENCODE_DST                             = 0x00008000U,     // KHR
        VIDEO_ENCODE_SRC                             = 0x00010000U,     // KHR
        SHADER_DEVICE_ADDRESS                        = 0x00020000U,
        ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY = 0x00080000U,     // KHR
        ACCELERATION_STRUCTURE_STORAGE               = 0x00100000U,     // KHR
        SAMPLER_DESCRIPTOR                           = 0x00200000U,     // EXT
        RESOURCE_DESCRIPTOR                          = 0x00400000U,     // EXT
        MICROMAP_BUILD_INPUT_READ_ONLY               = 0x00800000U,     // EXT
        MICROMAP_STORAGE                             = 0x01000000U,     // EXT
        // beta extension -> need VK_ENABLE_BETA_EXTENSIONS #define
        // EXECUTION_GRAPH_SCRATCH                      = 0x02000000,      // AMDX
        PUSH_DESCRIPTORS_DESCRIPTOR                  = 0x04000000U,     // EXT
        MAX_ENUM                                     = 0x7FFFFFFFU
    };
    enum class Image: uint32 {
        TRANSFER_SRC                        = 0x00000001U,
        TRANSFER_DST                        = 0x00000002U,
        SAMPLED                             = 0x00000004U,
        STORAGE                             = 0x00000008U,
        COLOR_ATTACHMENT                    = 0x00000010U,
        DEPTH_STENCIL_ATTACHMENT            = 0x00000020U,
        TRANSIENT_ATTACHMENT                = 0x00000040U,
        INPUT_ATTACHMENT                    = 0x00000080U,
        FRAGMENT_SHADING_RATE_ATTACHMENT    = 0x00000100U,  // KHR
        SHADING_RATE_IMAGE                  = 0x00000100U,  // NV
        FRAGMENT_DENSITY_MAP                = 0x00000200U,  // EXT
        VIDEO_DECODE_DST                    = 0x00000400U,  // KHR
        VIDEO_DECODE_SRC                    = 0x00000800U,  // KHR
        VIDEO_DECODE_DPB                    = 0x00001000U,  // KHR
        VIDEO_ENCODE_DST                    = 0x00002000U,  // KHR
        VIDEO_ENCODE_SRC                    = 0x00004000U,  // KHR
        VIDEO_ENCODE_DPB                    = 0x00008000U,  // KHR
        INVOCATION_MASK                     = 0x00040000U,  // HUAWEI
        ATTACHMENT_FEEDBACK_LOOP            = 0x00080000U,  // EXT
        SAMPLE_WEIGHT                       = 0x00100000U,  // QCOM
        SAMPLE_BLOCK_MATCH                  = 0x00200000U,  // QCOM
        HOST_TRANSFER                       = 0x00400000U,
        VIDEO_ENCODE_QUANTIZATION_DELTA_MAP = 0x02000000U,  // KHR
        VIDEO_ENCODE_EMPHASIS_MAP           = 0x04000000U,  // KHR
        MAX_ENUM                            = 0x7FFFFFFFU
    };

    inline constexpr Buffer operator|(Buffer a, Buffer b) noexcept { return static_cast<Buffer>(static_cast<uint32>(a) | static_cast<uint32>(b)); }
    inline constexpr bool   operator&(Buffer a, Buffer b) noexcept { return static_cast<bool>(static_cast<uint32>(a) & static_cast<uint32>(b));   }

    inline constexpr Image operator|(Image a, Image b) noexcept { return static_cast<Image> (static_cast<uint32>(a) | static_cast<uint32>(b)); }
}
namespace Layout {
    enum class Image: int32 {
        UNDEFINED                                  = 0         ,
        GENERAL                                    = 1         ,
        COLOR_ATTACHMENT_OPTIMAL                   = 2         ,
        DEPTH_STENCIL_ATTACHMENT_OPTIMAL           = 3         ,
        DEPTH_STENCIL_READ_ONLY_OPTIMAL            = 4         ,
        SHADER_READ_ONLY_OPTIMAL                   = 5         ,
        TRANSFER_SRC_OPTIMAL                       = 6         ,
        TRANSFER_DST_OPTIMAL                       = 7         ,
        PREINITIALIZED                             = 8         ,
        DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL = 1000117000,
        DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL = 1000117001,
        DEPTH_ATTACHMENT_OPTIMAL                   = 1000241000,
        DEPTH_READ_ONLY_OPTIMAL                    = 1000241001,
        STENCIL_ATTACHMENT_OPTIMAL                 = 1000241002,
        STENCIL_READ_ONLY_OPTIMAL                  = 1000241003,
        READ_ONLY_OPTIMAL                          = 1000314000,
        ATTACHMENT_OPTIMAL                         = 1000314001,
        RENDERING_LOCAL_READ                       = 1000232000,
        PRESENT_SRC                                = 1000001002,    // KHR
        VIDEO_DECODE_DST                           = 1000024000,    // KHR
        VIDEO_DECODE_SRC                           = 1000024001,    // KHR
        VIDEO_DECODE_DPB                           = 1000024002,    // KHR
        SHARED_PRESENT                             = 1000111000,    // KHR
        FRAGMENT_DENSITY_MAP_OPTIMAL               = 1000218000,    // EXT
        FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL   = 1000164003,    // KHR
        SHADING_RATE_OPTIMAL                       = 1000164003,    // NV
        VIDEO_ENCODE_DST                           = 1000299000,    // KHR
        VIDEO_ENCODE_SRC                           = 1000299001,    // KHR
        VIDEO_ENCODE_DPB                           = 1000299002,    // KHR
        ATTACHMENT_FEEDBACK_LOOP_OPTIMAL           = 1000339000,    // EXT
        VIDEO_ENCODE_QUANTIZATION_MAP              = 1000553000,    // KHR
        MAX_ENUM = 0x7FFFFFFF
    };
}
namespace Property {
    enum class Memory: uint32 {
        DEVICE_LOCAL     = 0x00000001,
        HOST_VISIBLE     = 0x00000002,
        HOST_COHERENT    = 0x00000004,
        HOST_CACHED      = 0x00000008,
        LAZILY_ALLOCATED = 0x00000010,
        PROTECTED        = 0x00000020,
        DEVICE_COHERENT  = 0x00000040,  // AMD
        DEVICE_UNCACHED  = 0x00000080,  // AMD
        RDMA_CAPABLE     = 0x00000100,  // NV
        MAX_ENUM         = 0x7FFFFFFF
    };

    inline constexpr Memory operator|(Memory a, Memory b) noexcept { return static_cast<Memory>(static_cast<uint32>(a) | static_cast<uint32>(b)); }
}
namespace Type {
    enum: int32 {
        Image1D  = 0,
        Image2D  = 1,
        Image3D  = 2,
        MAX_ENUM = 0x7FFFFFFF
    };

    enum class ImageFormat: int32 {
        UNDEFINED                                  = 0,
        R4G4_UNORM_PACK8                           = 1,
        R4G4B4A4_UNORM_PACK16                      = 2,
        B4G4R4A4_UNORM_PACK16                      = 3,
        R5G6B5_UNORM_PACK16                        = 4,
        B5G6R5_UNORM_PACK16                        = 5,
        R5G5B5A1_UNORM_PACK16                      = 6,
        B5G5R5A1_UNORM_PACK16                      = 7,
        A1R5G5B5_UNORM_PACK16                      = 8,
        R8_UNORM                                   = 9,
        R8_SNORM                                   = 10,
        R8_USCALED                                 = 11,
        R8_SSCALED                                 = 12,
        R8_UINT                                    = 13,
        R8_SINT                                    = 14,
        R8_SRGB                                    = 15,
        R8G8_UNORM                                 = 16,
        R8G8_SNORM                                 = 17,
        R8G8_USCALED                               = 18,
        R8G8_SSCALED                               = 19,
        R8G8_UINT                                  = 20,
        R8G8_SINT                                  = 21,
        R8G8_SRGB                                  = 22,
        R8G8B8_UNORM                               = 23,
        R8G8B8_SNORM                               = 24,
        R8G8B8_USCALED                             = 25,
        R8G8B8_SSCALED                             = 26,
        R8G8B8_UINT                                = 27,
        R8G8B8_SINT                                = 28,
        R8G8B8_SRGB                                = 29,
        B8G8R8_UNORM                               = 30,
        B8G8R8_SNORM                               = 31,
        B8G8R8_USCALED                             = 32,
        B8G8R8_SSCALED                             = 33,
        B8G8R8_UINT                                = 34,
        B8G8R8_SINT                                = 35,
        B8G8R8_SRGB                                = 36,
        R8G8B8A8_UNORM                             = 37,
        R8G8B8A8_SNORM                             = 38,
        R8G8B8A8_USCALED                           = 39,
        R8G8B8A8_SSCALED                           = 40,
        R8G8B8A8_UINT                              = 41,
        R8G8B8A8_SINT                              = 42,
        R8G8B8A8_SRGB                              = 43,
        B8G8R8A8_UNORM                             = 44,
        B8G8R8A8_SNORM                             = 45,
        B8G8R8A8_USCALED                           = 46,
        B8G8R8A8_SSCALED                           = 47,
        B8G8R8A8_UINT                              = 48,
        B8G8R8A8_SINT                              = 49,
        B8G8R8A8_SRGB                              = 50,
        A8B8G8R8_UNORM_PACK32                      = 51,
        A8B8G8R8_SNORM_PACK32                      = 52,
        A8B8G8R8_USCALED_PACK32                    = 53,
        A8B8G8R8_SSCALED_PACK32                    = 54,
        A8B8G8R8_UINT_PACK32                       = 55,
        A8B8G8R8_SINT_PACK32                       = 56,
        A8B8G8R8_SRGB_PACK32                       = 57,
        A2R10G10B10_UNORM_PACK32                   = 58,
        A2R10G10B10_SNORM_PACK32                   = 59,
        A2R10G10B10_USCALED_PACK32                 = 60,
        A2R10G10B10_SSCALED_PACK32                 = 61,
        A2R10G10B10_UINT_PACK32                    = 62,
        A2R10G10B10_SINT_PACK32                    = 63,
        A2B10G10R10_UNORM_PACK32                   = 64,
        A2B10G10R10_SNORM_PACK32                   = 65,
        A2B10G10R10_USCALED_PACK32                 = 66,
        A2B10G10R10_SSCALED_PACK32                 = 67,
        A2B10G10R10_UINT_PACK32                    = 68,
        A2B10G10R10_SINT_PACK32                    = 69,
        R16_UNORM                                  = 70,
        R16_SNORM                                  = 71,
        R16_USCALED                                = 72,
        R16_SSCALED                                = 73,
        R16_UINT                                   = 74,
        R16_SINT                                   = 75,
        R16_SFLOAT                                 = 76,
        R16G16_UNORM                               = 77,
        R16G16_SNORM                               = 78,
        R16G16_USCALED                             = 79,
        R16G16_SSCALED                             = 80,
        R16G16_UINT                                = 81,
        R16G16_SINT                                = 82,
        R16G16_SFLOAT                              = 83,
        R16G16B16_UNORM                            = 84,
        R16G16B16_SNORM                            = 85,
        R16G16B16_USCALED                          = 86,
        R16G16B16_SSCALED                          = 87,
        R16G16B16_UINT                             = 88,
        R16G16B16_SINT                             = 89,
        R16G16B16_SFLOAT                           = 90,
        R16G16B16A16_UNORM                         = 91,
        R16G16B16A16_SNORM                         = 92,
        R16G16B16A16_USCALED                       = 93,
        R16G16B16A16_SSCALED                       = 94,
        R16G16B16A16_UINT                          = 95,
        R16G16B16A16_SINT                          = 96,
        R16G16B16A16_SFLOAT                        = 97,
        R32_UINT                                   = 98,
        R32_SINT                                   = 99,
        R32_SFLOAT                                 = 100,
        R32G32_UINT                                = 101,
        R32G32_SINT                                = 102,
        R32G32_SFLOAT                              = 103,
        R32G32B32_UINT                             = 104,
        R32G32B32_SINT                             = 105,
        R32G32B32_SFLOAT                           = 106,
        R32G32B32A32_UINT                          = 107,
        R32G32B32A32_SINT                          = 108,
        R32G32B32A32_SFLOAT                        = 109,
        R64_UINT                                   = 110,
        R64_SINT                                   = 111,
        R64_SFLOAT                                 = 112,
        R64G64_UINT                                = 113,
        R64G64_SINT                                = 114,
        R64G64_SFLOAT                              = 115,
        R64G64B64_UINT                             = 116,
        R64G64B64_SINT                             = 117,
        R64G64B64_SFLOAT                           = 118,
        R64G64B64A64_UINT                          = 119,
        R64G64B64A64_SINT                          = 120,
        R64G64B64A64_SFLOAT                        = 121,
        B10G11R11_UFLOAT_PACK32                    = 122,
        E5B9G9R9_UFLOAT_PACK32                     = 123,
        D16_UNORM                                  = 124,
        X8_D24_UNORM_PACK32                        = 125,
        D32_SFLOAT                                 = 126,
        S8_UINT                                    = 127,
        D16_UNORM_S8_UINT                          = 128,
        D24_UNORM_S8_UINT                          = 129,
        D32_SFLOAT_S8_UINT                         = 130,
        BC1_RGB_UNORM_BLOCK                        = 131,
        BC1_RGB_SRGB_BLOCK                         = 132,
        BC1_RGBA_UNORM_BLOCK                       = 133,
        BC1_RGBA_SRGB_BLOCK                        = 134,
        BC2_UNORM_BLOCK                            = 135,
        BC2_SRGB_BLOCK                             = 136,
        BC3_UNORM_BLOCK                            = 137,
        BC3_SRGB_BLOCK                             = 138,
        BC4_UNORM_BLOCK                            = 139,
        BC4_SNORM_BLOCK                            = 140,
        BC5_UNORM_BLOCK                            = 141,
        BC5_SNORM_BLOCK                            = 142,
        BC6H_UFLOAT_BLOCK                          = 143,
        BC6H_SFLOAT_BLOCK                          = 144,
        BC7_UNORM_BLOCK                            = 145,
        BC7_SRGB_BLOCK                             = 146,
        ETC2_R8G8B8_UNORM_BLOCK                    = 147,
        ETC2_R8G8B8_SRGB_BLOCK                     = 148,
        ETC2_R8G8B8A1_UNORM_BLOCK                  = 149,
        ETC2_R8G8B8A1_SRGB_BLOCK                   = 150,
        ETC2_R8G8B8A8_UNORM_BLOCK                  = 151,
        ETC2_R8G8B8A8_SRGB_BLOCK                   = 152,
        EAC_R11_UNORM_BLOCK                        = 153,
        EAC_R11_SNORM_BLOCK                        = 154,
        EAC_R11G11_UNORM_BLOCK                     = 155,
        EAC_R11G11_SNORM_BLOCK                     = 156,
        ASTC_4x4_UNORM_BLOCK                       = 157,
        ASTC_4x4_SRGB_BLOCK                        = 158,
        ASTC_5x4_UNORM_BLOCK                       = 159,
        ASTC_5x4_SRGB_BLOCK                        = 160,
        ASTC_5x5_UNORM_BLOCK                       = 161,
        ASTC_5x5_SRGB_BLOCK                        = 162,
        ASTC_6x5_UNORM_BLOCK                       = 163,
        ASTC_6x5_SRGB_BLOCK                        = 164,
        ASTC_6x6_UNORM_BLOCK                       = 165,
        ASTC_6x6_SRGB_BLOCK                        = 166,
        ASTC_8x5_UNORM_BLOCK                       = 167,
        ASTC_8x5_SRGB_BLOCK                        = 168,
        ASTC_8x6_UNORM_BLOCK                       = 169,
        ASTC_8x6_SRGB_BLOCK                        = 170,
        ASTC_8x8_UNORM_BLOCK                       = 171,
        ASTC_8x8_SRGB_BLOCK                        = 172,
        ASTC_10x5_UNORM_BLOCK                      = 173,
        ASTC_10x5_SRGB_BLOCK                       = 174,
        ASTC_10x6_UNORM_BLOCK                      = 175,
        ASTC_10x6_SRGB_BLOCK                       = 176,
        ASTC_10x8_UNORM_BLOCK                      = 177,
        ASTC_10x8_SRGB_BLOCK                       = 178,
        ASTC_10x10_UNORM_BLOCK                     = 179,
        ASTC_10x10_SRGB_BLOCK                      = 180,
        ASTC_12x10_UNORM_BLOCK                     = 181,
        ASTC_12x10_SRGB_BLOCK                      = 182,
        ASTC_12x12_UNORM_BLOCK                     = 183,
        ASTC_12x12_SRGB_BLOCK                      = 184,
        G8B8G8R8_422_UNORM                         = 1000156000,
        B8G8R8G8_422_UNORM                         = 1000156001,
        G8_B8_R8_3PLANE_420_UNORM                  = 1000156002,
        G8_B8R8_2PLANE_420_UNORM                   = 1000156003,
        G8_B8_R8_3PLANE_422_UNORM                  = 1000156004,
        G8_B8R8_2PLANE_422_UNORM                   = 1000156005,
        G8_B8_R8_3PLANE_444_UNORM                  = 1000156006,
        R10X6_UNORM_PACK16                         = 1000156007,
        R10X6G10X6_UNORM_2PACK16                   = 1000156008,
        R10X6G10X6B10X6A10X6_UNORM_4PACK16         = 1000156009,
        G10X6B10X6G10X6R10X6_422_UNORM_4PACK16     = 1000156010,
        B10X6G10X6R10X6G10X6_422_UNORM_4PACK16     = 1000156011,
        G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16 = 1000156012,
        G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16  = 1000156013,
        G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16 = 1000156014,
        G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16  = 1000156015,
        G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16 = 1000156016,
        R12X4_UNORM_PACK16                         = 1000156017,
        R12X4G12X4_UNORM_2PACK16                   = 1000156018,
        R12X4G12X4B12X4A12X4_UNORM_4PACK16         = 1000156019,
        G12X4B12X4G12X4R12X4_422_UNORM_4PACK16     = 1000156020,
        B12X4G12X4R12X4G12X4_422_UNORM_4PACK16     = 1000156021,
        G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16 = 1000156022,
        G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16  = 1000156023,
        G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16 = 1000156024,
        G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16  = 1000156025,
        G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16 = 1000156026,
        G16B16G16R16_422_UNORM                     = 1000156027,
        B16G16R16G16_422_UNORM                     = 1000156028,
        G16_B16_R16_3PLANE_420_UNORM               = 1000156029,
        G16_B16R16_2PLANE_420_UNORM                = 1000156030,
        G16_B16_R16_3PLANE_422_UNORM               = 1000156031,
        G16_B16R16_2PLANE_422_UNORM                = 1000156032,
        G16_B16_R16_3PLANE_444_UNORM               = 1000156033,
        G8_B8R8_2PLANE_444_UNORM                   = 1000330000,
        G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16  = 1000330001,
        G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16  = 1000330002,
        G16_B16R16_2PLANE_444_UNORM                = 1000330003,
        A4R4G4B4_UNORM_PACK16                      = 1000340000,
        A4B4G4R4_UNORM_PACK16                      = 1000340001,
        ASTC_4x4_SFLOAT_BLOCK                      = 1000066000,
        ASTC_5x4_SFLOAT_BLOCK                      = 1000066001,
        ASTC_5x5_SFLOAT_BLOCK                      = 1000066002,
        ASTC_6x5_SFLOAT_BLOCK                      = 1000066003,
        ASTC_6x6_SFLOAT_BLOCK                      = 1000066004,
        ASTC_8x5_SFLOAT_BLOCK                      = 1000066005,
        ASTC_8x6_SFLOAT_BLOCK                      = 1000066006,
        ASTC_8x8_SFLOAT_BLOCK                      = 1000066007,
        ASTC_10x5_SFLOAT_BLOCK                     = 1000066008,
        ASTC_10x6_SFLOAT_BLOCK                     = 1000066009,
        ASTC_10x8_SFLOAT_BLOCK                     = 1000066010,
        ASTC_10x10_SFLOAT_BLOCK                    = 1000066011,
        ASTC_12x10_SFLOAT_BLOCK                    = 1000066012,
        ASTC_12x12_SFLOAT_BLOCK                    = 1000066013,
        A1B5G5R5_UNORM_PACK16                      = 1000470000,
        A8_UNORM                                   = 1000470001,
        PVRTC1_2BPP_UNORM_BLOCK_IMG                = 1000054000,
        PVRTC1_4BPP_UNORM_BLOCK_IMG                = 1000054001,
        PVRTC2_2BPP_UNORM_BLOCK_IMG                = 1000054002,
        PVRTC2_4BPP_UNORM_BLOCK_IMG                = 1000054003,
        PVRTC1_2BPP_SRGB_BLOCK_IMG                 = 1000054004,
        PVRTC1_4BPP_SRGB_BLOCK_IMG                 = 1000054005,
        PVRTC2_2BPP_SRGB_BLOCK_IMG                 = 1000054006,
        PVRTC2_4BPP_SRGB_BLOCK_IMG                 = 1000054007,
        R16G16_SFIXED5                             = 1000464000,    // NV
        MAX_ENUM                                   = 0x7FFFFFFF
    };
    enum class ColorSpace: int32 {
        SRGB_NONLINEAR          = 0,                // KHR
        DISPLAY_P3_NONLINEAR    = 1000104001,       // EXT
        EXTENDED_SRGB_LINEAR    = 1000104002,       // EXT
        DISPLAY_P3_LINEAR       = 1000104003,       // EXT
        DCI_P3_NONLINEAR        = 1000104004,       // EXT
        BT709_LINEAR            = 1000104005,       // EXT
        BT709_NONLINEAR         = 1000104006,       // EXT
        BT2020_LINEAR           = 1000104007,       // EXT
        HDR10_ST2084            = 1000104008,       // EXT
        HDR10_HLG               = 1000104010,       // EXT
        ADOBERGB_LINEAR         = 1000104011,       // EXT
        ADOBERGB_NONLINEAR      = 1000104012,       // EXT
        PASS_THROUGH            = 1000104013,       // EXT
        EXTENDED_SRGB_NONLINEAR = 1000104014,       // EXT
        DISPLAY_NATIVE          = 1000213000,       // AMD
        MAX_ENUM                = 0x7FFFFFFF
    };

    struct Extent2D {
        uint32 width  = { };
        uint32 height = { };
    };
    struct Extent3D {
        uint32 width  = { };
        uint32 height = { };
        uint32 depth  = { };
    };
}