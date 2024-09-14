vec4 rayMarchCloud(in vec3 worldPos, in vec3 worldDir, inout float tDepth)
{
    float viewHeight = length(worldPos);
    float tMin, tMax;
    vec2 tTop2 = vec2(-1);
    vec2 tBottom2 = vec2(-1);
    vec3 planetCenter = vec3(0);
    if (rayIntersectSphereSolution(worldPos, worldDir, vec4(planetCenter, uCloudLayerTopRadiusKm), tTop2))
    {
        if (rayIntersectSphereSolution(worldPos, worldDir, vec4(planetCenter, uCloudLayerBottomRadiusKm), tBottom2))
        {
            float tempTop = all(greaterThan(tTop2, vec2(0))) ? min(tTop2.x, tTop2.y) : max(tTop2.x, tTop2.y);
            float tempBottom = all(greaterThan(tBottom2, vec2(0))) ? min(tBottom2.x, tBottom2.y) : max(tBottom2.x, tBottom2.y);

            if (all(greaterThan(tBottom2, vec2(0))))
                tempTop = max(0.0, min(tTop2.x, tTop2.y));

            tMin = min(tempBottom, tempTop);
            tMax = max(tempBottom, tempTop);
        }
        else
        {
            tMin = tTop2.x;
            tMax = tTop2.y;
        }
    }
    else
    {
        return vec4(0.0, 0.0, 0.0, 1.0);
    }
    tMin = max(tMin, 0.0);
    tMax = max(tMax, 0.0);

    if (tMax <= tMin || tMin > uTracingStartMaxDistanceKm || tMin > tDepth)
        return vec4(0.0, 0.0, 0.0, 1.0);

    float marchingDistance = min(tMax - tMin, uTracingMaxDistanceKm);
    tMax = min(tMin + uTracingMaxDistanceKm, tDepth);

    const uint stepCountUint = max(uCloudSampleCountMin, uint(uCloudSampleCountMax * saturate((tMax - tMin) * uInvDistanceToSampleCountMax)));
    const float stepCount = float(stepCountUint);
    const float stepT = (tMax - tMin) / stepCount;
    const float dtMeters = stepT * 1000.0;

    const float cosTheta = dot(-worldDir, uSunDirection);
    const float phase = dualLobPhase(uPhaseG0, uPhaseG1, uPhaseBlend, cosTheta);

    ParticipatingMediaPhaseContext pmpc = setupParticipatingMediaPhaseContext(phase, uMsPhaseFactor);

    float t = tMin + getBlueNoise(ivec2(gFullResCoord), osg_FrameNumber / 16 % 8) * stepT;
    vec3 luminance = vec3(0.0);
    vec3 transmittanceToView = vec3(1.0);

    for (uint i = 0; i < stepCountUint; ++i)
    {
        const vec3 samplePos = worldPos + worldDir * t;
        const float sampleHeight = length(samplePos);
        const float normalizedHeight = saturate((sampleHeight - uCloudLayerBottomRadiusKm) / uCloudLayerThicknessKm);
        const float density = sampleCloudDensity(samplePos, normalizedHeight);
        if (density <= 0.01)
        {
            t += stepT;
            continue;
        }

        vec3 transmittanceToLight0;
        {
            const vec3 upVector = samplePos / sampleHeight;
            const float viewZenithCos = dot(uSunDirection, upVector);
            vec2 sampleUV;
            transmittanceLutParametersToUV(sampleHeight, viewZenithCos, sampleUV);
            transmittanceToLight0 = texture(uTransmittanceLutTexture, sampleUV).rgb;
        }
        ParticipatingMediaContext pmc = setupParticipatingMediaContext(uAlbedo, density * uExtinction, uMsScattFactor, uMsExtinFactor, transmittanceToLight0);

        //if (density > 0.0)
        {
            if (t < tDepth)
                tDepth = t;
            const float maxTransmittanceToView = max(max(transmittanceToView.x, transmittanceToView.y), transmittanceToView.z);
            vec3 extinctionAcc[MS_COUNT];
            const float shadowLengthTest = uCloudShadowTracingMaxDistanceKm;
            const float shadowStepCount = float(uCloudShadowSampleCountMax);
            const float invShadowStepCount = 1.0 / shadowStepCount;
            const float shadowJitteringSeed = float(osg_FrameNumber / 16 % 8) + pseudoRandom(gFullResCoord);
            const float shadowJitterNorm = 0.5; //interleavedGradientNoise(gl_FragCoord.xy, shadowJitteringSeed) - 0.5;

            for (uint ms = 0; ms < MS_COUNT; ++ms)
                extinctionAcc[ms] = vec3(0.0);

            const float shadowDtMeter = shadowLengthTest * 1000.0;
            float previousNormT = 0.0;
            for (float shadowT = invShadowStepCount; shadowT <= 1.00001; shadowT += invShadowStepCount)
            {
                float currentNormT = shadowT * shadowT;
                const float detalNormT = currentNormT - previousNormT;
                const float extinctionFactor = detalNormT;
                const float shadowSampleDistance = shadowLengthTest * (previousNormT + detalNormT * shadowJitterNorm);
                const vec3 shadowSamplePos = samplePos + uSunDirection * shadowSampleDistance;
                const float shadowSampleNormalizedHeight = saturate((length(shadowSamplePos) - uCloudLayerBottomRadiusKm) / uCloudLayerThicknessKm);
                float shadowSampleDensity = sampleCloudDensity(shadowSamplePos, shadowSampleNormalizedHeight);
                previousNormT = currentNormT;

                if (shadowSampleDensity <= 0)
                    continue;
                
                ParticipatingMediaContext shadowPMC = setupParticipatingMediaContext(vec3(0), shadowSampleDensity * uExtinction, uMsScattFactor, uMsExtinFactor, vec3(0));

                for (uint ms = 0; ms < MS_COUNT; ++ms)
                    extinctionAcc[ms] += shadowPMC.extinctionCoeff[ms] * extinctionFactor;
            }

            for (uint ms = 0; ms < MS_COUNT; ++ms)
                pmc.transmittanceToLight0[ms] *= exp(-extinctionAcc[ms] * shadowDtMeter);
        }

        const vec3 distantLightLuminance = gDistantSkyLight * saturate(0.5 + normalizedHeight);
        for (uint ms = MS_COUNT - 1; ms >= 0; --ms)
        {
            const vec3 scatteringCoeff = pmc.scatteringCoeff[ms];
            const vec3 extinctionCoeff = pmc.extinctionCoeff[ms];
            const vec3 transmittanceToLight0 = pmc.transmittanceToLight0[ms];
            vec3 sunSkyLuminance = transmittanceToLight0 * uSunIntensity * pmpc.phase0[ms];
            sunSkyLuminance += (ms == 0 ? distantLightLuminance : vec3(0));

            const vec3 scatteredLuminance = sunSkyLuminance * scatteringCoeff;
            const vec3 safeExtinctionThreshold = vec3(0.000001);
            const vec3 safeExtinctionCoeff = max(safeExtinctionThreshold, extinctionCoeff);
            const vec3 safePathSegmentTransmittance = exp(-safeExtinctionCoeff * dtMeters);
            vec3 luminanceIntegral = (scatteredLuminance - scatteredLuminance * safePathSegmentTransmittance) / safeExtinctionCoeff;
            luminance += transmittanceToView * luminanceIntegral;

            if (ms == 0)
                transmittanceToView *= safePathSegmentTransmittance;
        }

        if (all(lessThan(transmittanceToView, uStopTracingTransmittanceThreshold)))
            break;

        t += stepT;
    }

    vec4 outColor = vec4(luminance, dot(transmittanceToView, vec3(1.0 / 3.0)));
    float slice = aerialPerspectiveDepthToSlice(tMin);
    float weight = 1.0;
    if (slice < 0.5)
    {
        weight = clamp(slice * 2.0, 0.0, 1.0);
        slice = 0.5;
    }
    float w = sqrt(slice / SLICE_COUNT);
    vec4 AP = weight * textureLod(uAerialPerspectiveLutTexture, vec3(uv, w), 0.0);
    AP.a = 1.0 - AP.a;
    outColor = vec4(AP.rgb * (1.0 - outColor.a) + AP.a * outColor.rgb, outColor.a);
    return outColor;
}