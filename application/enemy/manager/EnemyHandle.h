#pragma once

#include <cstdint>

/// @brief EnemyManagerが発行する敵の寿命を跨いで保持できる識別子
/// @details 実アドレスを保持しないため、削除済みのEnemyを再参照しない。
/// @note EnemyManagerの生成ごとに有効範囲が決まる。別のEnemyManagerへ持ち出してはならない。
struct EnemyHandle {
	std::uint64_t value = 0;

	/// @brief 識別子に未設定値が入っていないか判定
	/// @return valueが0以外の場合はtrue。存在確認にはEnemyManager::IsEnemyValid()を使用する。
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
