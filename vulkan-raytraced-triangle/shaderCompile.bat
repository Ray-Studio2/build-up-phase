glslc --target-env=vulkan1.3 "./shaders/raygen.rgen" -o "./shaders/raygen.spv"
glslc --target-env=vulkan1.3 "./shaders/bgMiss.rmiss" -o "./shaders/bgMiss.spv"
glslc --target-env=vulkan1.3 "./shaders/aoMiss.rmiss" -o "./shaders/aoMiss.spv"
glslc --target-env=vulkan1.3 "./shaders/cHit.rchit" -o "./shaders/cHit.spv"