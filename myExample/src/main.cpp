#include "graphics/GGeometry.h"
#include "vulkanSrc/vkContext.h"

int main() {
	std::vector<VertexInfo> vec_VertInfo{
		GCone({0.0, 0.0, 0.0}, 1000.0, 600.0, 30).getVertex(),
		GCone({3000.0, 0.0, 0.0}, 1000.0, 1000.0, 45).getVertex(),
		GCone({0.0, 3000.0, 0.0}, 1000.0, 3000.0, 30).getVertex(),
	};

	VertexInfo vertexInfo = mergVertexInfo(vec_VertInfo, false);

	VkContext::getInstance().setVertex(vertexInfo.vec_Vertex);
	VkContext::getInstance().setIndex(vertexInfo.vec_Index);

	VkContext::getInstance().init();

	if (!VkContext::getInstance().valid())
		return 1;

	VkContext::getInstance().startRenderLoop();

	return 0;
}
