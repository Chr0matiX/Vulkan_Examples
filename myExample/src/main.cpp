#include "graphics/GGeometry.h"
#include "vulkanSrc/vkContext.h"

int main() {
	GCone cone({0.0, 0.0, 0.0}, 1000.0, 600.0, 30);

	const auto & vertexInfo = cone.getVertex();
	VkContext::getInstance().setVertex(vertexInfo.vec_Vertex);
	VkContext::getInstance().setIndex(vertexInfo.vec_Index);

	VkContext::getInstance().init();

	if (!VkContext::getInstance().valid())
		return 1;

	VkContext::getInstance().startRenderLoop();

	return 0;
}
