#include "vulkanSrc/vkContext.h"

int main() {
	if (!VkContext::getInstance().valid())
		return 1;

	VkContext::getInstance().startRenderLoop();

	return 0;
}
