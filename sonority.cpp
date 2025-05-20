#include "mx/api/ScoreData.h"

#include "sonority.h"

Sonority::Sonority(const mx::api::NoteData& note_1, const mx::api::NoteData& note_2, const int rhythmic_hierarchy, const int id)
	: m_note_1{ note_1 }, m_note_2{ note_2 }, m_rhythmic_hierarchy{ rhythmic_hierarchy }, m_index{ id } {
	//m_compound_interval = get_interval(m_note_1, m_note_2, false);
	//m_simple_interval = m_compound_interval;
}

const bool Sonority::is_sonority_dissonant(const std::pair<std::vector<int>, std::vector<int>>& dissonant_intervals) const {
	// <scale degrees, semitones>
	return is_dissonant(m_note_1, m_note_2, dissonant_intervals);
}

void Sonority::build_motion_data(Sonority& next_sonority) {
	m_note_1_motion = get_interval(get_note_1(), next_sonority.get_note_1(), true); // Don't care about tritone leaps
	m_note_2_motion = get_interval(get_note_2(), next_sonority.get_note_2(), true);
}

const MotionType Sonority::get_motion_type() const {
	const int note_1_motion_type{ (m_note_1_motion.second > 0) ? 1 : ((m_note_1_motion.second < 0) ? -1 : 0) };
	const int note_2_motion_type{ (m_note_2_motion.second > 0) ? 1 : ((m_note_2_motion.second < 0) ? -1 : 0) };
	if (note_1_motion_type == note_2_motion_type) {
		if (note_1_motion_type == 0) {
			return stationary;
		}
		else {
			return similar;
		}
	}
	else {
		if (note_1_motion_type == 0 || note_2_motion_type == 0) {
			return oblique;
		}
		else {
			return contrary;
		}
	}
}

const Interval get_interval(const mx::api::NoteData& note_1, const mx::api::NoteData& note_2, const bool number_signed) {
	// <int, int>---<scale degree interval, semitone interval>
	// If signed, note_2 higher = positive
	// Returns compound interval

	if (note_1.isRest || note_2.isRest) {
		return { -1000, -1000 }; // -1000 = rest code
	}

	const int octave_difference{ note_2.pitchData.octave - note_1.pitchData.octave };
	const int scale_degree_interval{ static_cast<int>(note_2.pitchData.step) - static_cast<int>(note_1.pitchData.step) + 7 * octave_difference };

	// Check for tritones
	const std::map<mx::api::Step, int> semitones{ // Map each white note to a number, use alter to increment the number
		{ mx::api::Step::c, 0 },
		{ mx::api::Step::d, 2 },
		{ mx::api::Step::e, 4 },
		{ mx::api::Step::f, 5 },
		{ mx::api::Step::g, 7 },
		{ mx::api::Step::a, 9 },
		{ mx::api::Step::b, 11 },
	};
	const int note_1_semitones{ semitones.at(note_1.pitchData.step) + note_1.pitchData.alter }; // Add 12 to avoid negative values (C flat)
	const int note_2_semitones{ semitones.at(note_2.pitchData.step) + note_2.pitchData.alter };
	const int semitone_interval{ note_2_semitones - note_1_semitones + 12 * octave_difference };
	const Interval output{ scale_degree_interval, semitone_interval };

	if (number_signed) {
		return output;
	}
	else {
		return Interval{ abs(output.first), abs(output.second) };
	}
}

const bool is_dissonant(const mx::api::NoteData& note_1, const mx::api::NoteData& note_2,
	const std::pair<std::vector<int>, std::vector<int>>& dissonant_intervals
	= std::pair<std::vector<int>, std::vector<int>>{ std::vector<int>{1, 6}, std::vector<int>{} }) {
	// <scale degrees, semitones>

	// Exempt augmented unisons since raised/lowered leading tones are weird and glitchy
	// || (simple_interval.second % 12 == 6)
	// Treat the Aug 4 and Dim 5 as consonant if not involving bass.

	if (note_1.isRest || note_2.isRest) {
		return false;
	}

	Interval simple_interval{ get_interval(note_1, note_2, false) };
	simple_interval.first %= 7;
	simple_interval.second %= 12;

	if (std::find(dissonant_intervals.first.begin(), dissonant_intervals.first.end(), simple_interval.first) != dissonant_intervals.first.end()
		|| std::find(dissonant_intervals.second.begin(), dissonant_intervals.second.end(), simple_interval.second) != dissonant_intervals.second.end()) {
		// Find in parameter list
		return true;
	}

	return false;
}

const bool is_identical(const Sonority& sonority_1, const Sonority& sonority_2) {
	const bool note_1_indexentical{ (sonority_1.get_note_1().pitchData == sonority_2.get_note_1().pitchData) && (sonority_1.get_note_1().isRest == sonority_2.get_note_1().isRest) };
	const bool note_2_indexentical{ (sonority_1.get_note_2().pitchData == sonority_2.get_note_2().pitchData) && (sonority_1.get_note_2().isRest == sonority_2.get_note_2().isRest) };
	return note_1_indexentical && note_2_indexentical;
}