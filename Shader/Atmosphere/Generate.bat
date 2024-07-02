glslc -E TransmittanceLut.comp.glsl -o .\Generated\TransmittanceLut.comp.glsl
glslc -E MultiScatteringLut.comp.glsl -o ./Generated/MultiScatteringLut.comp.glsl
glslc -E SkyViewLut.comp.glsl -o ./Generated/SkyViewLut.comp.glsl
glslc -E AerialPerspectiveLut.comp.glsl -o ./Generated/AerialPerspectiveLut.comp.glsl
glslc -E RayMarching.frag.glsl -o ./Generated/RayMarching.frag.glsl
glslc -E SkyLightCubemap.comp.glsl -o ./Generated/SkyLightCubemap.comp.glsl