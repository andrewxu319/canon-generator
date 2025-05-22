
#include "exception.h"
#include "settings.h"
#include "file_reader.h"
#include "file_writer.h"
#include "sonority.h"
#include "canon.h"
#include "counterpoint_checker.h"

#include "mx/api/ScoreData.h"

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>

#define DEBUG
//#define SINGLE_SHIFT_CHECK

using Voice = std::vector<mx::api::NoteData>; // Condensed voice in terminology from create_voice_array()

const std::vector<int> alters_by_key(const int fifths) {
	std::vector<int> alters_array{ 0, 0, 0, 0, 0, 0, 0 };

	if (fifths < 0) {
		for (int i{ 0 }; i < -fifths; ++i) {
			--alters_array[(((i + 1) * 3) + 3) % 7];
		}
		return alters_array;
	}
	else if (fifths > 0) {
		for (int i{ 0 }; i < +fifths; ++i) {
			++alters_array[((i + 1) * 4) % 7 - 1];
		}
		return alters_array;
	}

	return alters_array;
}

const mx::api::DurationData ticks_to_duration_data(const int ticks, const int ticks_per_measure, const mx::api::TimeSignatureData& time_signature) {
	// TODO: CHECK FOR INTEGER DIVISION (tick size cannot be larger than beat size)
	const int dur32nd_per_tick{ 32 * time_signature.beats / ticks_per_measure / time_signature.beatType }; // Won't have anything for that durations are too small
	const int dur32nd_in_note{ ticks * dur32nd_per_tick };
	mx::api::DurationData duration{};
	duration.durationTimeTicks = ticks;
	std::map<int, std::pair<mx::api::DurationName, int>> map {
		{ 1, { mx::api::DurationName::dur32nd, 0 } },
		{ 2, { mx::api::DurationName::dur16th, 0 } },
		{ 3, { mx::api::DurationName::dur16th, 1 } },
		{ 4, { mx::api::DurationName::eighth, 0 } },
		{ 6, { mx::api::DurationName::eighth, 1 } },
		{ 7, { mx::api::DurationName::eighth, 2 } },
		{ 8, { mx::api::DurationName::quarter, 0 } },
		{ 12, { mx::api::DurationName::quarter, 1 } },
		{ 14, { mx::api::DurationName::quarter, 2 } },
		{ 16, { mx::api::DurationName::half, 0 } },
		{ 24, { mx::api::DurationName::half, 1 } },
		{ 28, { mx::api::DurationName::half, 2 } },
		{ 32, { mx::api::DurationName::whole, 0 } },
		{ 48, { mx::api::DurationName::whole, 1 } },
		{ 56, { mx::api::DurationName::whole, 2 } },
		{ 64, { mx::api::DurationName::breve, 0 } },
		{ 96, { mx::api::DurationName::breve, 1 } },
		{ 112, { mx::api::DurationName::breve, 2 } },
	};

	try {
		duration.durationName = map.at(dur32nd_in_note).first;
		duration.durationDots = map.at(dur32nd_in_note).second;
	}
	catch (...) { // TODO: specify exception type
		throw Exception{ "Invalid duration!" };
	}

	return duration;
}

const mx::api::NoteData create_rest(const int ticks, const int ticks_per_measure, const mx::api::TimeSignatureData& time_signature) {
	mx::api::NoteData rest{};
	rest.isRest = true;
	rest.durationData = ticks_to_duration_data(ticks, ticks_per_measure, time_signature);
	return rest;
}

const mx::api::MeasureData create_measure(const mx::api::NoteData& measure_long_rest, mx::api::MeasureData template_measure) {
	for (mx::api::StaffData& staff : template_measure.staves) {
		staff.voices.at(0).notes = std::vector<mx::api::NoteData>{};
		staff.voices.at(0).notes.emplace_back(measure_long_rest);
	}
	return template_measure;
}

const mx::api::DirectionData create_canon_label(const int warnings_count) {
	mx::api::DirectionData direction{};
	direction.words.emplace_back(mx::api::WordsData{});
	direction.words.back().text = "Warning count: " + std::to_string(warnings_count);
	return direction;
}

const mx::api::BarlineData create_barline() {
	mx::api::BarlineData barline{};
	barline.barlineType = mx::api::BarlineType::lightLight;
	barline.location = mx::api::HorizontalAlignment::right;
	return barline;
}

const mx::api::PartData extend_part_length(mx::api::PartData part, int additional_measures, const mx::api::TimeSignatureData& time_signature, const mx::api::NoteData& rest) {
	part.measures.back().barlines = std::vector<mx::api::BarlineData>{};

	if (additional_measures == 0) {
		return part;
	}

	mx::api::StaffData staff{ part.measures.at(0).staves.at(0) };
	staff.clefs = std::vector<mx::api::ClefData>{};

	staff.voices.at(0) = mx::api::VoiceData{};
	staff.voices.at(0).notes.emplace_back(rest);

	mx::api::MeasureData measure{};
	measure.timeSignature = time_signature;
	measure.timeSignature.isImplicit = true;
	measure.staves.emplace_back(staff);

	for (int i{ 0 }; i < additional_measures; ++i) { // Double length of part
		part.measures.emplace_back(measure);
	}

	return part;
}

const int get_last_key_before(const int ticks) {
	const std::vector<int> keys{ 1,2,3,4,6,7,8,12,14,16,24,28,32,48,56,64,96,112 }; // TODO: link this to map in ticks_to_duration_data https://www.geeksforgeeks.org/extract-keys-from-cpp-map/;
	return *std::lower_bound(keys.rbegin(), keys.rend(), ticks, std::greater<int>());
}

Voice split_note(mx::api::NoteData original_note, const int first_half_ticks, const int ticks_per_measure, const mx::api::TimeSignatureData& time_signature) {
	Voice output_voice{}; // Array of notes

	// First part
	output_voice.emplace_back(original_note);
	try {
		output_voice.back().durationData = ticks_to_duration_data(first_half_ticks, ticks_per_measure, time_signature);
		output_voice.back().isTieStart = true;

	}
	catch (...) { // TODO: specify exception
		output_voice.at(0).durationData.durationTimeTicks = first_half_ticks; // Truncate so everything still fits in the first measure. Don't care about the rest of durationData since it'll fix itself later
		const Voice reattempt_splitted_notes{ split_note(output_voice.at(0), get_last_key_before(first_half_ticks), ticks_per_measure, time_signature) };
		output_voice.erase(output_voice.begin());
		for (mx::api::NoteData reattempt_splitted_note : reattempt_splitted_notes) {
			output_voice.emplace_back(reattempt_splitted_note);
			output_voice.back().isTieStart = true; // Because this is actually the second to last at most (second part to be added)
		}
	}

	// Second part
	output_voice.emplace_back(original_note);
	if (original_note.isTieStart) {
		output_voice.back().isTieStart = true;
	}
	output_voice.back().isTieStop = true;
	output_voice.back().tickTimePosition = (output_voice.end()[-2].tickTimePosition + output_voice.end()[-2].durationData.durationTimeTicks) % ticks_per_measure;
	output_voice.back().durationData.durationTimeTicks = original_note.durationData.durationTimeTicks - first_half_ticks;
	try {
		output_voice.back().durationData = ticks_to_duration_data(output_voice.back().durationData.durationTimeTicks, ticks_per_measure, time_signature);
	}
	catch (...) { // TODO: specify exception
 		const Voice reattempt_splitted_notes{ split_note(output_voice.back(), get_last_key_before(original_note.durationData.durationTimeTicks - first_half_ticks), ticks_per_measure, time_signature) };
		output_voice.pop_back();
		for (mx::api::NoteData reattempt_splitted_note : reattempt_splitted_notes) {
			output_voice.emplace_back(reattempt_splitted_note);
		}
	}

	return output_voice;
}

const Voice split_rests(const int ticks, const int tick_time_position, const int ticks_per_measure, const mx::api::TimeSignatureData& time_signature) {
	mx::api::NoteData original_rest{};
	original_rest.isRest = true;
	original_rest.durationData.durationTimeTicks = ticks;
	original_rest.tickTimePosition = tick_time_position;
	Voice splitted_rests{ original_rest };
	if (splitted_rests.back().durationData.durationTimeTicks > ticks_per_measure) {
		while (splitted_rests.back().durationData.durationTimeTicks > ticks_per_measure) {
			Voice splitted_back{ split_note(splitted_rests.back(), ticks_per_measure, ticks_per_measure, time_signature) };
			splitted_rests.pop_back();
			for (mx::api::NoteData& rest : splitted_back) {
				splitted_rests.emplace_back(rest);
			}
		}
	}
	else {
		try {
			// splitted_rests.back() is just equal to original note
			splitted_rests.back().durationData = ticks_to_duration_data(splitted_rests.back().durationData.durationTimeTicks, ticks_per_measure, time_signature);
		}
		catch (...) { // TODO: specify exception
			splitted_rests = { split_note(splitted_rests.back(), get_last_key_before(ticks), ticks_per_measure, time_signature) };
		}
	}
	return splitted_rests;
}

const Voice shift(Voice voice, const int v_shift, const int h_shift, const std::vector<int>& key_signature, const ScaleDegree leading_tone, const bool minor_key, const int ticks_per_measure, const mx::api::TimeSignatureData& time_signature) { // h_shift in ticks. voice is explicitly a copy	
	// Vertical shift
	if (v_shift != 0) {
		for (mx::api::NoteData& note : voice) {
			// Diatonic transpose
			if (v_shift < -6 || v_shift > 6) {
				throw Exception{ "Vertical shift must be between -6 and 6! " + std::to_string(v_shift) + " is illegal.\n" };
			}

			note.pitchData.alter = note.pitchData.alter - key_signature.at(static_cast<int>(note.pitchData.step)); // Apply extra accidentals (in addition to key signature)

			const int destination_int_pre_mod{ static_cast<int>(note.pitchData.step) + v_shift };
			if (v_shift > 0) {
				note.pitchData.step = static_cast<mx::api::Step>(destination_int_pre_mod % 7);
				if (destination_int_pre_mod > 6) {
					++note.pitchData.octave;
				}
			}
			else if (v_shift < 0) {
				note.pitchData.step = static_cast<mx::api::Step>((destination_int_pre_mod + 21) % 7); // Adjust the 21 based on max number of extra octaves
				if (destination_int_pre_mod < 0) { // 6?
					--note.pitchData.octave;
				}
			}

			// Key signature
			if (minor_key && (note.pitchData.step == leading_tone.step)) {
				// If minor key note is the 7th
				note.pitchData.alter += key_signature.at(static_cast<int>(note.pitchData.step)) + 1; // Override normal accidental preservation
			}
			else {
				note.pitchData.alter += key_signature.at(static_cast<int>(note.pitchData.step));
			}
		}
	}

	if (h_shift != 0) {
		// TODO: Check to make sure second half of voice array is empty
		for (mx::api::NoteData& note : voice) {
			note.tickTimePosition = (note.tickTimePosition + h_shift) % ticks_per_measure;
		}

		// Insert starting rest(s)
		Voice splitted_starting_rests{ split_rests(h_shift, 0, ticks_per_measure, time_signature) };
		splitted_starting_rests.insert(splitted_starting_rests.end(), voice.begin(), voice.end()); // Concatinate the strings into splitted_rests. I know it's wonky
		voice = splitted_starting_rests;

		// Insert trailing rest(s)
		Voice splitted_trailing_rests{ split_rests(ticks_per_measure - h_shift % ticks_per_measure, h_shift, ticks_per_measure, time_signature) };
		for (const mx::api::NoteData& rest : splitted_trailing_rests) {
			voice.emplace_back(rest);
		}
	}

	//for (mx::api::NoteData& note : voice) {
	//	note.pitchData.octave += settings::octave_shift;
	//}

	return voice;
}

const mx::api::PartData voice_array_to_part(const mx::api::ScoreData& score, Voice voice, const int ticks_per_measure, const mx::api::TimeSignatureData& time_signature, const mx::api::NoteData& measure_long_rest, const int leader_length_measures) {
	mx::api::PartData part{ score.parts.at(0) }; // Copy
	part = extend_part_length(part, 2 * leader_length_measures - part.measures.size(), time_signature, measure_long_rest); // TODO: verify time signature doesn't change

	// Remap measures
	for (mx::api::MeasureData& measure : part.measures) { // Iterates through the measures
		measure.staves.at(0).voices.at(0) = mx::api::VoiceData{};
	}

	for (int i{ 0 }; i < part.measures.size(); ++i) {
		for (int j{ 0 }; j < ticks_per_measure;) { // Increment j BEFORE erasing the first voice in array
			const int ticks_to_barline{ ticks_per_measure - j };
			if (ticks_to_barline < voice.at(0).durationData.durationTimeTicks) {
				auto& splitted_notes{ split_note(voice.at(0), ticks_to_barline, ticks_per_measure, time_signature)};
				voice.erase(voice.begin());
				while (splitted_notes.size() > 0) {
					voice.insert(voice.begin(), splitted_notes.back()); // Insert in REVERSE ORDER so first in splitted_notes goes in front
					splitted_notes.pop_back();
				}
			}

			j += voice.at(0).durationData.durationTimeTicks;

			part.measures.at(i).staves.at(0).voices.at(0).notes.emplace_back(voice.at(0));
			voice.erase(voice.begin());

			if (voice.size() <= 0) {
				return part;
			}
		}
	}

	return part;
}

const mx::api::ScoreData create_output_score(mx::api::ScoreData score, const std::vector<mx::api::PartData>& parts) { // "score" is template score. add follower(s) to original score
	score.parts = std::vector<mx::api::PartData>{}; // Clear parts list in score
	for (int i{ 0 }; i < parts.size(); ++i) {
		mx::api::PartData follower_part{ parts.at(i)}; // Makes copy
		follower_part.uniqueId = 'P' + std::to_string(i + 1); // P1 is leader, P2 is first follower
		score.parts.emplace_back(follower_part);
	}

	return score; // temp
}

const int second_highest_notes_by_tick_lengths(const std::vector<std::vector<mx::api::NoteData>>& notes_by_tick) {
	// Get second highest notes-by-tick length (need two voices to counterpoint)
	std::vector<int> notes_by_tick_lengths{};
	for (const Voice& voice : notes_by_tick) {
		notes_by_tick_lengths.push_back(voice.size());
	}
	std::sort(notes_by_tick_lengths.begin(), notes_by_tick_lengths.end(), std::greater<int>());
	return notes_by_tick_lengths.at(1);
}

const std::vector<int> create_rhythmic_hierarchy_array(const int ticks_per_measure, const mx::api::TimeSignatureData& time_signature) {
	std::vector<int> rhythmic_hierarchy(ticks_per_measure); // All 0

	// CLEAN UP ALL THIS CODE---MAKE FUNCTION "increment_every()"

	// Simple meter
	if (time_signature.beatType == 4) {
		for (int division_size{ 2 }; division_size <= ticks_per_measure; division_size *= 2) {
			if (ticks_per_measure % division_size == 0) {
				for (int i{ 0 }; i < ticks_per_measure; ++i) {
					if (i % division_size == 0) {
						++rhythmic_hierarchy.at(i);
					}
				}
			}
			else if (ticks_per_measure % 3 == 0) {
				for (int i{ 0 }; i < ticks_per_measure; ++i) {
					if (i % (3 * division_size / 2) == 0) {
						++rhythmic_hierarchy.at(i);
					}
				}
			}
			else {
				break;
			}
		}
	}

	// Compound meter
	else if (time_signature.beatType == 8) {
		const int ticks_per_eighth{ ticks_per_measure / time_signature.beats };
		for (int division_size{ 2 }; division_size <= ticks_per_eighth; division_size *= 2) { // Subdivide anything smaller than 8 into two
			for (int i{ 0 }; i < ticks_per_measure; ++i) {
				if (i % division_size == 0) {
					++rhythmic_hierarchy.at(i);
				}
			}
		}
		for (int division_size{ ticks_per_eighth }; division_size < ticks_per_measure;) { // Eighth notes and 9/8
			if ((ticks_per_measure / division_size) % 3 == 0) {
				division_size *= 3;
				for (int i{ 0 }; i < ticks_per_measure; ++i) {
					if (i % (division_size) == 0) {
						++rhythmic_hierarchy.at(i);
					}
				}
			}
			else if ((ticks_per_measure / division_size) % 2 == 0) {
				division_size *= 2;
				for (int i{ 0 }; i < ticks_per_measure; ++i) {
					if (i % (division_size) == 0) {
						++rhythmic_hierarchy.at(i);
					}
				}
			}
		}
	}

	return rhythmic_hierarchy;
}

const ScaleDegree get_tonic(const int fifths, const bool minor_key) {
	int step{ 4 * fifths };
	if (minor_key) {
		step -= 2;
	}
	step = (step % 7 + 7) % 7; // Fix octaves. Extra +7 to fix negative output for %
	const int alter{ alters_by_key(fifths).at(step) };
	return ScaleDegree{ static_cast<mx::api::Step>(step), alter };
}

std::vector<Canon> generate_canons_for_new_voice(std::vector<Canon>& template_canons_array, const Voice& leader, const int leader_length_ticks, const int ticks_per_measure, const int ticks_per_beat, const std::vector<int>& rhythmic_hierarchy_array, const int rhythmic_hierarchy_max_depth, const int rhythmic_hierarchy_of_beat, const std::vector<int>& key_signature, const ScaleDegree& tonic, const ScaleDegree& dominant, const ScaleDegree& leading_tone, const bool minor_key, const mx::api::TimeSignatureData& time_signature, const mx::api::NoteData& measure_long_rest) {
	std::vector<Canon> valid_canons_for_current_voice{};
	
	for (Canon& template_canon : template_canons_array) {

#ifndef SINGLE_SHIFT_CHECK
		for (int h_shift{ template_canon.get_max_h_shift() + 1}; h_shift < leader_length_ticks * settings::min_tightness; ++h_shift) {

			// Maximum h_shift increment because tick sizes are unpredictable for some reason
			if (h_shift % (ticks_per_beat / settings::h_shift_increments_per_beat) != 0) {
				continue;
			}

			for (int v_shift{ -1 }; v_shift >= -6; --v_shift) {
#endif // SINGLE_SHIFT_CHECK

#ifdef SINGLE_SHIFT_CHECK
				const int v_shift{ -3 };
				const int h_shift{ 32 };
				if (h_shift > leader_length_ticks) {
					std::cout << "h_shift is too large\n";
				}
#endif // SINGLE_SHIFT_CHECK

				// Create follower (LOOP THIS)
				Canon canon{ template_canon.texture(), template_canon.get_max_h_shift()};
				const Voice follower{ shift(leader, v_shift, h_shift, key_signature, leading_tone, minor_key, ticks_per_measure, time_signature) }; // const
				canon.texture().emplace_back(follower);

				// Append empty measures to leader so both voices have the same number of complete measures
				for (int i{ 0 }; i < canon.texture().size() - 1; ++i) { // Skip last one (follower)
					for (int j{ 0 }; j < (h_shift / ticks_per_measure + 1); ++j) {
						canon.texture().at(i).emplace_back(measure_long_rest);
					}
				}

				// Check counterpoint
				check_counterpoint(canon, ticks_per_measure, tonic, dominant, leading_tone, rhythmic_hierarchy_array, rhythmic_hierarchy_max_depth, rhythmic_hierarchy_of_beat);
				// This will change member variables in canon

#ifdef SINGLE_SHIFT_CHECK
				valid_canons_for_current_voice.emplace_back(Canon{ canon.texture(), h_shift });
#endif // SINGLE_SHIFT_CHECK

#ifndef SINGLE_SHIFT_CHECK
				//const double score{ errors_count + settings::warning_weight * warnings_count }; // Not sure when you would need to use this
				if (canon.get_error_count() > 0 || canon.get_warning_count() > settings::warning_threshold) {
#ifdef DEBUG
					//std::cout << "Canon rejected! At h_shift = " << h_shift << ", v_shift = " << v_shift << "\n\n";
#endif // DEBUG
					continue;
					//valid_canons_for_current_voice.emplace_back(Canon{ canon.texture(), h_shift });
				}
				else {
					valid_canons_for_current_voice.emplace_back(Canon{ canon.texture(), h_shift });
#ifdef DEBUG
					//std::cout << "Valid canon! At h_shift = " << h_shift << ", v_shift = " << v_shift << "\n\n";
#endif // DEBUG
				}
			}
		}
#endif // SINGLE_SHIFT_CHECK
	}

	return valid_canons_for_current_voice;
}

int main() {
	try {
		// Read file
		const std::string& xml{ read_file(settings::read_file_path) };
		const mx::api::ScoreData& score{ get_score_object(xml) };
		const Voice leader{ create_voice_array(score) };

		// Key signature
		const int fifths{ score.parts.at(0).measures.at(0).keys.at(0).fifths };
		const bool minor_key{ settings::minor_key };
		const std::vector<int> key_signature{ alters_by_key(fifths) };

		// Scale degrees
		const ScaleDegree tonic{ get_tonic(fifths, settings::minor_key) };
		const int tonic_step{ static_cast<int>(tonic.step) };
		const int dominant_step{ (tonic_step + 4) % 7 };
		const ScaleDegree dominant{ static_cast<mx::api::Step>(dominant_step), alters_by_key(fifths).at(dominant_step) + 1 };
		const int leading_tone_step{ (tonic_step + 6) % 7 };
		const ScaleDegree leading_tone{ static_cast<mx::api::Step>(leading_tone_step), alters_by_key(fifths).at(leading_tone_step) + 1 };

		// Get time signature
		const mx::api::TimeSignatureData& time_signature{ score.parts.at(0).measures.at(0).timeSignature };

		// Calculate ticks per measure and initialize other variables
		const mx::api::NoteData& last_note{ score.parts.at(0).measures.at(0).staves.at(0).voices.at(0).notes.back() }; // Use original score, before horizontal shifting
		const int ticks_per_measure{ last_note.tickTimePosition + last_note.durationData.durationTimeTicks };
		const int leader_length_measures{ static_cast<int>(score.parts.at(0).measures.size()) };
		const int leader_length_ticks{ ticks_per_measure * leader_length_measures };
		const mx::api::NoteData measure_long_rest{ create_rest(ticks_per_measure, ticks_per_measure, time_signature) };
		const mx::api::MeasureData empty_measure{ create_measure(measure_long_rest, score.parts.at(0).measures.back()) };
		const mx::api::BarlineData double_barline{ create_barline() };

		// Generate rhythmic hierarchy
		const std::vector<int> rhythmic_hierarchy_array{ create_rhythmic_hierarchy_array(ticks_per_measure, time_signature) };
		const int rhythmic_hierarchy_max_depth{ *std::max_element(rhythmic_hierarchy_array.begin(), rhythmic_hierarchy_array.end()) };
		const int ticks_per_beat{ ticks_per_measure / time_signature.beats };
		const int rhythmic_hierarchy_of_beat{ rhythmic_hierarchy_array.at(ticks_per_beat) }; // Second beat is always on the weakest beat hierarchies

		// TODO: FORBID VOICE CROSSINGS

		/*
		* have an array store a list of partdata
		* add to the list of partdata every iteration
		* clear template_canons_array , add to it every iteration
		* the function would take existing voices and create a new follower
		* loop until template_canons_array is empty or when max_voices is reached
		*/
		int valid_canons_counter{ 0 };
		std::vector<Canon> valid_canons{};
		std::vector<Canon> template_canons_array{ Canon{std::vector<Voice>{leader}, 0} };

		for (int i{ 0 }; i < settings::max_voices - 1; ++i) {
			const std::vector<Canon> valid_canons_for_current_voice{ generate_canons_for_new_voice(template_canons_array, leader, leader_length_ticks, ticks_per_measure, ticks_per_beat, rhythmic_hierarchy_array, rhythmic_hierarchy_max_depth, rhythmic_hierarchy_of_beat, key_signature, tonic, dominant, leading_tone, minor_key, time_signature, measure_long_rest) };
			template_canons_array = valid_canons_for_current_voice;

			valid_canons.reserve(valid_canons.size() + valid_canons_for_current_voice.size());
			for (const Canon& canon : valid_canons_for_current_voice) {
				valid_canons.emplace_back(canon);
			}
		}
		valid_canons_counter = valid_canons.size();

		if (valid_canons_counter == 0) {
			std::cout << "No valid canons found!\n";
			return 0;
		}

		std::vector<mx::api::PartData> valid_canons_parts_sequence(settings::max_voices);
		for (Canon& canon : valid_canons) {
			// Create musicxml
			const mx::api::PartData& leader_part{ score.parts.at(0) };

			std::vector<mx::api::PartData> parts_array(settings::max_voices);
			parts_array.at(0) = leader_part;
			for (int i{ 1 }; i < settings::max_voices; ++i) { // Skip first because first is leader part
				if (i >= canon.texture().size()) {
					canon.texture().emplace_back(Voice{});
					for (int j{ 0 }; j < 2 * leader_length_measures; ++j) {
						canon.texture().back().emplace_back(measure_long_rest); // Add one empty measure for now. Extend the part later
					}
				}
				parts_array.at(i) = voice_array_to_part(score, canon.texture().at(i), ticks_per_measure, time_signature, measure_long_rest, leader_length_measures); // Need a function to fix barlines
			}

			// Extend every part to the same length
			int longest_part_size{ 0 };
			for (const mx::api::PartData& part : parts_array) { // Get longest part
				if (part.measures.size() > longest_part_size) {
					longest_part_size = part.measures.size();
				}
			}
			for (mx::api::PartData& part : parts_array) {
				part = extend_part_length(part, longest_part_size - part.measures.size(), time_signature, measure_long_rest);
			}

			// FIGURE OUT HOW TO GET WARNINGS COUNT FOR EACH
			//// Add text labels
			parts_array.at(0).measures.at(0).staves.at(0).directions.emplace_back(create_canon_label(canon.get_warning_count()));

			// Push parts in canon into final valid_canons_parts_sequence
			for (int part{ 0 }; part < settings::max_voices; ++part) {
				for (const mx::api::MeasureData& measure : parts_array.at(part).measures) {
					// For each part, add each note to valid_canons_sequence
					valid_canons_parts_sequence.at(part).measures.emplace_back(measure);
				}
				valid_canons_parts_sequence.at(part).measures.back().barlines.push_back(double_barline);
				for (int i{ 0 }; i < settings::measures_separation_between_output_canons; ++i) {
					valid_canons_parts_sequence.at(part).measures.emplace_back(empty_measure);
				}
			}
		}

		// Remove extra time signatures and clefs
		for (mx::api::PartData& part : valid_canons_parts_sequence) {
			for (int i{ 1 }; i < part.measures.size(); ++i) {
				// Start at second measure
				mx::api::MeasureData& measure{ part.measures.at(i) };
				//measure.barlines = std::vector<mx::api::BarlineData>{};
				measure.timeSignature.isImplicit = true;
				for (mx::api::StaffData& staff: measure.staves) {
					staff.clefs = std::vector<mx::api::ClefData>{};
				}
			}
		}

		std::vector<mx::api::DirectionData>& directions{ valid_canons_parts_sequence.at(0).measures.at(0).staves.at(0).directions };
		directions.emplace_back(mx::api::DirectionData{});
		directions.back().words.emplace_back(mx::api::WordsData{});
		directions.back().words.back().text = std::to_string(valid_canons_counter) + " valid canons\n";

		const mx::api::ScoreData output_score{ create_output_score(score, valid_canons_parts_sequence) }; // add follower(s) to original score

		//// Write file
		write_file(output_score, settings::write_file_path); // output_score
	}
	catch (const Exception& exception) {
		std::cout << exception.getError();
	}

	return 0;
}


//#include "mx/api/DocumentManager.h"
//#include "mx/api/ScoreData.h"
//
//#include <string>
//#include <iostream>
//#include <cstdint>
//#include <sstream>
//
//#define MX_IS_A_SUCCESS 0
//#define MX_IS_A_FAILURE 1
//
//int main(int argc, const char* argv[]) // main1
//{
//    const std::string xml = read_file();
//
//    using namespace mx::api;
//
//    // create a reference to the singleton which holds documents in memory for us
//    auto& mgr = DocumentManager::getInstance();
///
//    // place the xml from above into a stream object
//    std::istringstream istr{ xml };
//
//    // ask the document manager to parse the xml into memory for us, returns a document ID.
//    const auto documentID = mgr.createFromStream(istr);
//
//    // get the structural representation of the score from the document manager
//    const auto score = mgr.getData(documentID);
//
//    // we need to explicitly destroy the document from memory
//    mgr.destroyDocument(documentID);
//
//    // make sure we have exactly one part
//    if (score.parts.size() != 1)
//    {
//        return MX_IS_A_FAILURE;
//    }
//
//    // drill down into the data structure to retrieve the note
//    const auto& part = score.parts.at(0);
//    const auto& measure = part.measures.at(0);
//    const auto& staff = measure.staves.at(0);
//    const auto& voice = staff.voices.at(0);
//    const auto& note = voice.notes.at(0);
//
//    if (note.durationData.durationName != DurationName::whole)
//    {
//        return MX_IS_A_FAILURE;
//    }
//
//    if (note.pitchData.step != Step::c)
//    {
//        return MX_IS_A_FAILURE;
//    }
//
//    std::cout << note.pitchData.alter;
//
//    return MX_IS_A_SUCCESS;
//}