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
	Sonority::Sonority(const mx::api::NoteData& note_1, const mx::api::NoteData& note_2, const int rhythmic_hierarchy, const int id);

	const bool is_dissonant()const;
	void build_motion_data(Sonority& next_sonority);
	const MotionType get_motion_type() const;

	const int get_id() const {
		return m_id;
	}

	const mx::api::NoteData& get_note_1() const {
		return m_note_1;
	}

	const mx::api::NoteData& get_note_2() const {
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
		return m_compound_interval;
	}

	const Interval get_simple_interval() const {
		return m_simple_interval;
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
	const int m_id{}; // FIX
	const mx::api::NoteData& m_note_1{ mx::api::NoteData{} };
	const mx::api::NoteData& m_note_2{ mx::api::NoteData{} };
	const Interval m_compound_interval{ get_interval(m_note_1, m_note_2, false) };
	const Interval m_simple_interval{ m_compound_interval.first % 7, m_compound_interval.second % 12};
	Interval m_note_1_motion{ 0, 0 };
	Interval m_note_2_motion{ 0, 0 };
	int m_rhythmic_hierarchy{ 0 }; // 0 = weakest beat (tick).
};

using SonorityArray = std::vector<Sonority>;

const bool is_dissonant(const mx::api::NoteData& note_1, const mx::api::NoteData& note_2);
const bool is_identical(const Sonority& sonority_1, const Sonority& sonority_2);
