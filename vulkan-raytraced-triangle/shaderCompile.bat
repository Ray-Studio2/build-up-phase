glslc --target-env=vulkan1.2 "./shaders/raygen.rgen" -o "./shaders/raygen.spv"
glslc --target-env=vulkan1.2 "./shaders/bgMiss.rmiss" -o "./shaders/bgMiss.spv"
glslc --target-env=vulkan1.2 "./shaders/aoMiss.rmiss" -o "./shaders/aoMiss.spv"
glslc --target-env=vulkan1.2 "./shaders/cHit.rchit" -o "./shaders/cHit.spv"