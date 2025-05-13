#include "mx/api/ScoreData.h"

#include "sonority.h"

Sonority::Sonority(const mx::api::NoteData& note_1, const mx::api::NoteData& note_2, const int rhythmic_hierarchy)
	: m_note_1{ note_1 }, m_note_2{ note_2 }, m_rhythmic_hierarchy{ rhythmic_hierarchy } {
	m_interval = interval(m_note_1, m_note_2);
}

void Sonority::build_movement_data(Sonority& next_sonority) {
	m_note_1_movement = interval(next_sonority.get_note_1(), get_note_1(), true).first; // Don't care about tritone leaps
	m_note_2_movement = interval(next_sonority.get_note_2(), get_note_2(), true).first;
}

const std::pair<int, bool> interval(const mx::api::NoteData& note_1, const mx::api::NoteData& note_2, const bool number_signed) { // Second (bool)---tritone. If signed, note_2 higher = positive
	if (note_1.isRest || note_2.isRest) {
		return { -1000, false }; // -1000 = rest code
	}

	std::pair<int, bool> output{};

	const int signed_number{ (static_cast<int>(note_2.pitchData.step) + 7 * (note_2.pitchData.octave - note_1.pitchData.octave)) - static_cast<int>(note_1.pitchData.step) };

	// Check for tritones
	const std::map<mx::api::Step, int> note_codes{ // Map each white note to a number, use alter to increment the number
		{ mx::api::Step::c, 0 },
		{ mx::api::Step::d, 2 },
		{ mx::api::Step::e, 4 },
		{ mx::api::Step::f, 5 },
		{ mx::api::Step::g, 7 },
		{ mx::api::Step::a, 9 },
		{ mx::api::Step::b, 11 },
	};
	const int note_1_code{ (note_codes.at(note_1.pitchData.step) + note_1.pitchData.alter + 12) % 12 }; // Add 12 to avoid negative values (C flat)
	const int note_2_code{ (note_codes.at(note_2.pitchData.step) + note_2.pitchData.alter + 12) % 12 };
	if (abs(note_2_code - note_1_code) == 6) {
		output = { signed_number, true };
	}
	else {
		output = { signed_number, false };
	}

	if (number_signed) {
		output.first = abs(output.first);
	}
	return output;
}

const bool is_identical(const Sonority& sonority_1, const Sonority& sonority_2) {
	const bool note_1_identical{ (sonority_1.get_note_1().pitchData == sonority_2.get_note_1().pitchData) && (sonority_1.get_note_1().isRest == sonority_2.get_note_1().isRest) };
	const bool note_2_identical{ (sonority_1.get_note_2().pitchData == sonority_2.get_note_2().pitchData) && (sonority_1.get_note_2().isRest == sonority_2.get_note_2().isRest) };
	return note_1_identical && note_2_identical;
}