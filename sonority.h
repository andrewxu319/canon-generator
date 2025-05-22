#pragma once

#include "exception.h"

#include "mx/api/ScoreData.h"

using Interval = std::pair<int, int>;

enum MotionType {
	contrary,
	oblique,
	similar,
	stationary
};

const Interval get_interval(const mx::api::NoteData& note_1, const mx::api::NoteData& note_2, const bool number_signed);

class Sonority {
public:
	Sonority::Sonority(const mx::api::NoteData& note_1, const mx::api::NoteData& note_2, const int rhythmic_hierarchy, const int index);

	const bool Sonority::is_sonority_dissonant(const std::pair<std::vector<int>, std::vector<int>>& dissonant_intervals
		= std::pair<std::vector<int>, std::vector<int>>{ std::vector<int>{1, 6}, std::vector<int>{} }) const;
	void build_motion_data(Sonority& next_sonority);
	const MotionType get_motion_type() const;

	const int get_index() const {
		return m_index;
	}

	const mx::api::NoteData& get_note_1() const {
		return m_note_1;
	}

	const mx::api::NoteData& get_note_2() const {
		return m_note_2;
	}

	mx::api::NoteData& note_1() {
		return m_note_1;
	}

	mx::api::NoteData& note_2() {
		return m_note_2;
	}

	const mx::api::NoteData& get_note(const int voice) const {
		if (voice == 0) {
			return m_note_1;
		} else
		if (voice == 1) {
			return m_note_2;
		}
		else {
			throw Exception("Invalid voice index!");
		}
	}

	const Interval get_compound_interval() const {
		return get_interval(m_note_1, m_note_2, false);
	}

	const Interval get_signed_compound_interval() const {
		return get_interval(m_note_1, m_note_2, true);
	}

	const Interval get_simple_interval() const {
		return Interval{ get_compound_interval().first % 7, get_compound_interval().second % 12 };
	}

	const Interval get_signed_simple_interval() const {
		return Interval{ (get_signed_compound_interval().first % 7 + 7) % 7, (get_signed_compound_interval().second % 12 + 12) % 12 };
	}

	const Interval get_note_1_motion() const {
		return m_note_1_motion;
	}

	const Interval get_note_2_motion() const {
		return m_note_2_motion;
	}

	const Interval get_note_motion(const int voice) const {
		if (voice == 0) {
			return m_note_1_motion;
		} else
		if (voice == 1) {
			return m_note_2_motion;
		}
		else {
			throw Exception("Invalid voice index!");
		}
	}

	const int get_rhythmic_hierarchy() const {
		return m_rhythmic_hierarchy;
	}

private:
	const int m_index{}; // FIX
	mx::api::NoteData m_note_1{ mx::api::NoteData{} };
	mx::api::NoteData m_note_2{ mx::api::NoteData{} };
	//const Interval m_compound_interval{ get_interval(m_note_1, m_note_2, false) };
	//const Interval m_signed_compound_interval{ get_interval(m_note_1, m_note_2, true) }; // positive = note_1 lower
	//const Interval m_simple_interval{ m_compound_interval.first % 7, m_compound_interval.second % 12 };
	//const Interval m_signed_simple_interval{ (m_signed_compound_interval.first % 7 + 7) % 7, (m_signed_compound_interval.second % 12 + 12) % 12 }; // positive = note_1 lower
	Interval m_note_1_motion{ 0, 0 };
	Interval m_note_2_motion{ 0, 0 };
	int m_rhythmic_hierarchy{ 0 }; // 0 = weakest beat (tick).
};

using SonorityArray = std::vector<Sonority>;

const bool is_dissonant(const mx::api::NoteData& note_1, const mx::api::NoteData& note_2, const std::pair<std::vector<int>, std::vector<int>>& dissonant_intervals);
const bool is_identical(const Sonority& sonority_1, const Sonority& sonority_2);
