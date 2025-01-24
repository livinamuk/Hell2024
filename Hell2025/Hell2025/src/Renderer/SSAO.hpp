#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>

#define SSAO_KERNEL_SIZE 16
#define SSAO_NOISE_SIZE 16

namespace SSAO {

    inline std::vector<glm::vec3> GenerateSSAOKernel(int kernelSize) {
        std::vector<glm::vec3> ssaoKernel;
        ssaoKernel.reserve(kernelSize);
        for (int i = 0; i < kernelSize; ++i) {
            glm::vec3 sample(
                glm::linearRand(-1.0f, 1.0f),
                glm::linearRand(-1.0f, 1.0f),
                glm::linearRand(0.0f, 1.0f)
            );
            sample = glm::normalize(sample);
            sample *= glm::linearRand(0.1f, 1.0f);
            float scale = float(i) / kernelSize;
            scale = glm::mix(0.1f, 1.0f, scale * scale);
            sample *= scale;
            ssaoKernel.push_back(sample);
        }
        return ssaoKernel;
    }
    
    inline std::vector<glm::vec3> GenerateNoise(int noiseSize) {
        std::vector<glm::vec3> noiseTexture;
        noiseTexture.reserve(noiseSize * noiseSize);
        for (int i = 0; i < noiseSize * noiseSize; ++i) {
            glm::vec3 noise(
                glm::linearRand(-1.0f, 1.0f),
                glm::linearRand(-1.0f, 1.0f),
                0.0f
            );
            noiseTexture.push_back(noise);
        }
        return noiseTexture;
    }

}