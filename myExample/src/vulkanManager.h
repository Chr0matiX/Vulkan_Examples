#pragma once

#include "vulkan/vulkan.h"

class CVkMana {
private:
	static CVkMana * m_VkManaInst;
	VkInstance m_VkInst = VK_NULL_HANDLE;

private:
	CVkMana() = default;
	CVkMana(const CVkMana &) = delete;
	CVkMana(CVkMana &&) = delete;
	CVkMana & operator=(const CVkMana &) = delete;
	CVkMana & operator=(CVkMana &&) = delete;

	static const CVkMana & getInst();

public:
};
