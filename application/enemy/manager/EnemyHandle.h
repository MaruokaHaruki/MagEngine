#pragma once

#include <cstdint>

// EnemyManager が発行する、Enemy の寿命を跨いで保存できる識別子。
// 実アドレスを保持しないため、削除済み Enemy を再参照しない。
struct EnemyHandle {
	std::uint64_t value = 0;

	[[nodiscard]] bool IsValid() const {
		return value != 0;
	}

	bool operator==(const EnemyHandle &other) const {
		return value == other.value;
	}

	bool operator!=(const EnemyHandle &other) const {
		return !(*this == other);
	}
};
