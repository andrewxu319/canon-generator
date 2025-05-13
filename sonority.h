#pragma once

#include "mx/api/ScoreData.h"

class Sonority {
public:
	Sonority::Sonority(const mx::api::NoteData& note_1, const mx::api::NoteData& note_2, const int rhythmic_hierarchy);

	void build_movement_data(Sonority& next_sonority);

	const mx::api::NoteData& get_note_1() const {
		return m_note_1;
	}

	const mx::api::NoteData& get_note_2() const {
		return m_note_2;
	}

	const std::pair<int, bool> get_interval() const {
		return m_interval;
	}

	const int get_note_1_movement() const {
		return m_note_1_movement;
	}

	const int get_note_2_movement() const {
		return m_note_2_movement;
	}

	const int get_rhythmic_hierarchy() const {
		return m_rhythmic_hierarchy;
	}

private:
	const mx::api::NoteData& m_note_1{ mx::api::NoteData{} };
	const mx::api::NoteData& m_note_2{ mx::api::NoteData{} };
	std::pair<int, bool> m_interval{};
	int m_note_1_movement{ 0 };
	int m_note_2_movement{ 0 };
	int m_rhythmic_hierarchy{ 0 }; // 0 = weakest beat (tick).
};

const std::pair<int, bool> interval(const mx::api::NoteData& note_1, const mx::api::NoteData& note_2, const bool number_signed = false);
const bool is_identical(const Sonority& sonority_1, const Sonority& sonority_2);
