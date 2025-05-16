
#include "exception.h"
#include "settings.h"
#include "file_reader.h"
#include "file_writer.h"
#include "sonority.h"
#include "counterpoint_checker.h"
#include "mx/api/ScoreData.h"

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>

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

const mx::api::PartData extend_part_length(mx::api::PartData part, int additional_measures, const mx::api::TimeSignatureData& time_signature, const int ticks_per_measure) {
	part.measures.back().barlines = std::vector<mx::api::BarlineData>{};

	mx::api::StaffData staff{ part.measures.at(0).staves.at(0) };
	staff.clefs = std::vector<mx::api::ClefData>{};
	
	mx::api::NoteData rest{};
	rest.isRest = true;
	rest.durationData = ticks_to_duration_data(ticks_per_measure, ticks_per_measure, time_signature);

	staff.voices.at(0) = mx::api::VoiceData{};
	staff.voices.at(0).notes.emplace_back(rest);

	for (int i{ 0 }; i < additional_measures; ++i) { // Double length of part
		mx::api::MeasureData measure{};
		measure.timeSignature = time_signature;
		measure.timeSignature.isImplicit = true;
		measure.staves.emplace_back(staff);
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
		output_voice.at(0).durationData = ticks_to_duration_data(first_half_ticks, ticks_per_measure, time_signature);
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
	try {
		output_voice.back().durationData = ticks_to_duration_data(original_note.durationData.durationTimeTicks - first_half_ticks, ticks_per_measure, time_signature);
	}
	catch (...) { // TODO: specify exception
		const Voice reattempt_splitted_notes{ split_note(output_voice.back(), get_last_key_before(original_note.durationData.durationTimeTicks - first_half_ticks), ticks_per_measure, time_signature) };
		for (mx::api::NoteData reattempt_splitted_note : reattempt_splitted_notes) {
			output_voice.emplace_back(reattempt_splitted_note);
		}
	}

	return output_voice;
}

const Voice shift(Voice voice, const int v_shift, const int h_shift, const int fifths, const int ticks_per_measure, const mx::api::TimeSignatureData& time_signature) { // h_shift in ticks. voice is explicitly a copy
	// Vertical shift
	if (v_shift != 0) {
		for (mx::api::NoteData& note : voice) {
			// Diatonic transpose
			if (v_shift < -6 || v_shift > 6) {
				throw Exception{ "Vertical shift must be between -6 and 6! " + std::to_string(v_shift) + " is illegal.\n" };
			}
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

			// // Key signature
			note.pitchData.alter = alters_by_key(fifths)[static_cast<int>(note.pitchData.step)];
		}
	}

	if (h_shift != 0) {
		// TODO: Check to make sure second half of voice array is empty
		for (mx::api::NoteData& note : voice) {
			note.tickTimePosition = (note.tickTimePosition + h_shift) % ticks_per_measure;
		}

		mx::api::NoteData original_rest{};
		original_rest.isRest = true;
		original_rest.durationData.durationTimeTicks = h_shift;
		Voice splitted_rests{ original_rest };
		if (splitted_rests.back().durationData.durationTimeTicks > ticks_per_measure) {
			while (splitted_rests.back().durationData.durationTimeTicks > ticks_per_measure) {
				Voice splitted_back{ split_note(splitted_rests.back(), ticks_per_measure, ticks_per_measure, time_signature) };
				splitted_rests.pop_back();
				for (mx::api::NoteData rest : splitted_back) {
					splitted_rests.emplace_back(rest);
				}
			}
		}
		else{
			try {
				// splitted_rests.back() is just equal to original note
				splitted_rests.back().durationData = ticks_to_duration_data(splitted_rests.back().durationData.durationTimeTicks, ticks_per_measure, time_signature);
			}
			catch (...) { // TODO: specify exception
				splitted_rests = { split_note(splitted_rests.back(), get_last_key_before(h_shift), ticks_per_measure, time_signature) };
			}
		}
		
		//try {
		//	splitted_rests.back().durationData = ticks_to_duration_data(splitted_rests.back().durationData.durationTimeTicks, ticks_per_measure, time_signature);
		//	voice.insert(voice.begin(), rest);
		//}
		//catch (...) { // TODO: specify exception
		//	const Voice splitted_rests{ split_note(rest, get_last_key_before(h_shift), ticks_per_measure, time_signature) };
		//	for (mx::api::NoteData splitted_rest : splitted_rests) {
		//		voice.insert(voice.begin(), splitted_rest);
		//	}
		//}
		splitted_rests.insert(splitted_rests.end(), voice.begin(), voice.end()); // Concatinate the strings into splitted_rests. I know it's wonky
		voice = splitted_rests;
		//for (mx::api::NoteData rest : splitted_rests) {
		//	voice.insert(voice.begin(), rest);
		//}
	}

	return voice;
}

const mx::api::PartData voice_array_to_part(const mx::api::ScoreData& score, Voice voice, const int ticks_per_measure, const mx::api::TimeSignatureData& time_signature) {
	mx::api::PartData part{ score.parts.at(0) }; // Copy
	part = extend_part_length(part, part.measures.size(), time_signature, ticks_per_measure); // TODO: verify time signature doesn't change

	// Remap measures
	for (int i{ 0 }; i < part.measures.size(); ++i) { // Iterates through the measures
		part.measures.at(i).staves.at(0).voices.at(0) = mx::api::VoiceData{};

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
	for (std::size_t i{ 0 }; i < parts.size(); ++i) {
		mx::api::PartData follower_part{ parts[i] }; // Makes copy
		follower_part.uniqueId = 'P' + std::to_string(i + 1); // P1 is leader, P2 is first follower
		score.parts.emplace_back(follower_part);
	}
	return score; // temp
}

const int second_highest_notes_by_tick_lengths(const std::vector<std::vector<mx::api::NoteData>>& notes_by_tick_for_all_voices) {
	// Get second highest notes-by-tick length (need two voices to counterpoint)
	std::vector<int> notes_by_tick_lengths{};
	for (const Voice& voice : notes_by_tick_for_all_voices) {
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

int main() {
	try {
		// // Read file
		const std::string& xml{ read_file(settings::read_file_path) };
		const mx::api::ScoreData& score{ get_score_object(xml) };
		const int fifths{ score.parts.at(0).measures.at(0).keys.at(0).fifths }; // Key signature
		const Voice leader{ create_voice_array(score) };

		// Get time signature
		const mx::api::TimeSignatureData& time_signature{ score.parts.at(0).measures.at(0).timeSignature };

		// Calculate ticks per measure
		const mx::api::NoteData& last_note{ score.parts.at(0).measures.at(0).staves.at(0).voices.at(0).notes.back() }; // Use original score, before horizontal shifting
		const int ticks_per_measure{ last_note.tickTimePosition + last_note.durationData.durationTimeTicks };

		// Create follower (LOOP THIS)
		std::vector<Voice> voices_array{};
		voices_array.emplace_back(leader);
		// TODO: FORBID VOICE CROSSINGS

		// LOOP HERE (and multiple followers)
		int v_shift{ 0 };
		int h_shift{ 8 };
		const Voice follower{ shift(leader, v_shift, h_shift, fifths, ticks_per_measure, time_signature) }; // const
		voices_array.emplace_back(follower);

		// For every voice, generate per-tick note data. MAKES COPIES
		std::vector<std::vector<mx::api::NoteData>> notes_by_tick_for_all_voices{}; // Indexed by tick. Ordering of voices doesn't really matter
		for (Voice voice : voices_array) {
			std::vector<mx::api::NoteData> notes_by_tick_for_voice{};
			for (const mx::api::NoteData& note : voice) {
				for (int i{ 0 }; i < note.durationData.durationTimeTicks; ++i) { // Append as many times as the number of ticks the note lasts for
					notes_by_tick_for_voice.emplace_back(note);
				}
			}
			notes_by_tick_for_all_voices.emplace_back(notes_by_tick_for_voice);
		}

		// Generate rhythmic hierarchy
		const std::vector<int> rhythmic_hierarchy_array{create_rhythmic_hierarchy_array(ticks_per_measure, time_signature)};
		const int rhythmic_hierarchy_max_depth{ *std::max_element(rhythmic_hierarchy_array.begin(), rhythmic_hierarchy_array.end()) };
		const int ticks_per_beat{ ticks_per_measure / time_signature.beats };
		const int rhythmic_hierarchy_of_beat{ rhythmic_hierarchy_array.at(ticks_per_beat) }; // Second beat is always on the weakest beat hierarchies

	// FOR EACH PAIR OF VOICES {
	// (different pairs of voices will have different strippings)
		// Generate raw unstripped sonority array
		Voice& voice_1{ notes_by_tick_for_all_voices.at(0) }; // temp
		Voice& voice_2{ notes_by_tick_for_all_voices.at(1) }; // temp
		SonorityArray raw_sonority_array{};
		for (int i{ 0 }; i < second_highest_notes_by_tick_lengths(notes_by_tick_for_all_voices); ++i) {
			raw_sonority_array.emplace_back(Sonority{ voice_1.at(i), voice_2.at(i), rhythmic_hierarchy_array.at(i % ticks_per_measure), i });
		}

		// Generate downbeat sonority arrays
		std::vector<SonorityArray> sonority_arrays{};
		for (int depth{ 0 }; depth <= rhythmic_hierarchy_max_depth; ++depth) {
			SonorityArray unstripped_sonority_array{};
			for (int i{ 0 }; i < raw_sonority_array.size(); ++i) {
				if (depth <= raw_sonority_array.at(i).get_rhythmic_hierarchy()) {
					unstripped_sonority_array.emplace_back(raw_sonority_array.at(i));
				}
			}

			SonorityArray stripped_sonority_array{};
			stripped_sonority_array.emplace_back(unstripped_sonority_array.at(0));
			for (int i{ 0 }; i < unstripped_sonority_array.size() - 1; ++i) { // Always push the first
				if (!is_identical(unstripped_sonority_array.at(i), unstripped_sonority_array.at(i + 1))) {
					stripped_sonority_array.emplace_back(unstripped_sonority_array.at(i + 1));
				}
			}

			for (int i{ 0 }; i < stripped_sonority_array.size() - 1; ++i) {
				stripped_sonority_array.at(i).build_motion_data(stripped_sonority_array.at(i + 1));
			}

			sonority_arrays.emplace_back(stripped_sonority_array);
		}

		// Check counterpoint
		const auto grade{ check_counterpoint(sonority_arrays, ticks_per_measure, rhythmic_hierarchy_array, rhythmic_hierarchy_max_depth, rhythmic_hierarchy_of_beat) }; // Return type is a pair of lists of error and warning messages

	// }

		// Create musicxml
		const mx::api::PartData& original_leader_part{ score.parts.at(0) };
		//const mx::api::PartData leader_part{ extend_part_length(original_leader_part, original_leader_part.measures.size(), original_leader_part.measures.at(0).timeSignature, ticks_per_measure) };
		const mx::api::PartData leader_part{ original_leader_part };

		std::vector<mx::api::PartData> parts_array;
		parts_array.emplace_back(leader_part);
		for (int i{ 1 }; i < voices_array.size(); ++i) { // Skip first because first is leader part
			parts_array.emplace_back(voice_array_to_part(score, voices_array.at(i), ticks_per_measure, time_signature)); // Need a function to fix barlines
		}

		// Extend every part to the same length
		int longest_part_size{ 0 };
		for (const mx::api::PartData& part : parts_array) { // Get longest part
			if (part.measures.size() > longest_part_size) {
				longest_part_size = part.measures.size();
			}
		}
		for (mx::api::PartData& part : parts_array) {
			part = extend_part_length(part, longest_part_size - part.measures.size(), time_signature, ticks_per_measure);
		}

		const mx::api::ScoreData output_score{ create_output_score(score, parts_array) }; // add follower(s) to original score

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