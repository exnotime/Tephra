#include "LightEnvironment.h"

LightEnvironment::LightEnvironment() {}
LightEnvironment::~LightEnvironment() {}

void LightEnvironment::Init(const VkMemory* texMem, const VkMemory* bufferMem, uint32_t maxPointLights, uint32_t maxSpotLights, uint32_t maxDirLights) {

}
void LightEnvironment::Update() {

}
void LightEnvironment::SetIBL(const std::string& irradiance, const std::string& radiance) {

}
void LightEnvironment::Clear() {

}
void LightEnvironment::AddDirLight(const DirLight& dl) {

}
void LightEnvironment::AddPointLight(const PointLight& pl) {

}
void LightEnvironment::TransferStaticLights() {

}
void LightEnvironment::TransferDynamicLights(std::vector<DirLight>& dirLights, std::vector<PointLight>& pointLights) {

}