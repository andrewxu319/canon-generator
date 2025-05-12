
#include "exception.h"
#include "settings.h"
#include "file_reader.h"
#include "file_writer.h"
#include "mx/api/ScoreData.h"

#include <iostream>
#include <string>
#include <vector>
#include <map>

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
	/*switch (dur32nd_in_note) {
	case 1:
		duration.durationName = mx::api::DurationName::dur32nd;
		break;
	case 2:
		duration.durationName = mx::api::DurationName::dur16th;
		break;
	case 3:
		duration.durationName = mx::api::DurationName::dur16th;
		duration.durationDots = 1;
		break;
	case 4:
		duration.durationName = mx::api::DurationName::eighth;
		break;
	case 6:
		duration.durationName = mx::api::DurationName::eighth;
		duration.durationDots = 1;
		break;
	case 7:
		duration.durationName = mx::api::DurationName::eighth;
		duration.durationDots = 2;
		break;
	case 8:
		duration.durationName = mx::api::DurationName::quarter;
		break;
	case 12:
		duration.durationName = mx::api::DurationName::quarter;
		duration.durationDots = 1;
		break;
	case 14:
		duration.durationName = mx::api::DurationName::quarter;
		duration.durationDots = 2;
		break;
	case 16:
		duration.durationName = mx::api::DurationName::half;
		break;
	case 24:
		duration.durationName = mx::api::DurationName::half;
		duration.durationDots = 1;
		break;
	case 28:
		duration.durationName = mx::api::DurationName::half;
		duration.durationDots = 2;
		break;
	case 32:
		duration.durationName = mx::api::DurationName::whole;
		break;
	case 48:
		duration.durationName = mx::api::DurationName::whole;
		duration.durationDots = 1;
		break;
	case 56:
		duration.durationName = mx::api::DurationName::whole;
		duration.durationDots = 2;
		break;
	case 64:
		duration.durationName = mx::api::DurationName::breve;
		break;
	case 96:
		duration.durationName = mx::api::DurationName::breve;
		duration.durationDots = 1;
		break;
	case 112:
		duration.durationName = mx::api::DurationName::breve;
		duration.durationDots = 2;
		break;
	default:
		throw Exception{ "Invalid duration!" };
	}*/

	//const int ticks_per_quarter{ ticks_per_measure * time_signature.beatType / 4 / time_signature.beats }; // ticks_per_measure / time_signature.beats * (time_signature.beatType / 4)
	//const double num_of_quarters{ static_cast<double>(ticks) / ticks_per_quarter }
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
				note.pitchData.step = static_cast<mx::api::Step>(destination_int_pre_mod + 7);
				if (destination_int_pre_mod < 6) {
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

		mx::api::NoteData rest{};
		rest.isRest = true;
		rest.durationData = ticks_to_duration_data(h_shift, ticks_per_measure, time_signature);
		voice.insert(voice.begin(), rest);
	}

	return voice;
}

const mx::api::PartData double_part_length(mx::api::PartData part, int part_length, const mx::api::TimeSignatureData& time_signature) {
	part.measures.back().barlines = std::vector<mx::api::BarlineData>{};

	mx::api::StaffData staff{ part.measures.at(0).staves.at(0) };
	staff.clefs = std::vector<mx::api::ClefData>{};
	staff.voices.at(0) = mx::api::VoiceData{};
	for (int i{ 0 }; i < part_length; ++i) { // Double length of part
		mx::api::MeasureData measure{};
		measure.timeSignature = time_signature;
		measure.timeSignature.isImplicit = true;
		measure.staves.emplace_back(staff);
		part.measures.emplace_back(measure);
	}

	return part;
}

const mx::api::PartData voice_array_to_part(const mx::api::ScoreData& score, Voice voice, const int ticks_per_measure, const mx::api::TimeSignatureData& time_signature) {
	mx::api::PartData part{ score.parts.at(0) }; // Copy
	part = double_part_length(part, part.measures.size(), time_signature); // TODO: verify time signature doesn't change

	// Remap measures
	for (int i{ 0 }; i < part.measures.size(); ++i) { // Iterates through the measures
		part.measures.at(i).staves.at(0).voices.at(0) = mx::api::VoiceData{};

		for (int j{ 0 }; j < ticks_per_measure;) { // Increment j BEFORE erasing the first voice in array
			const int ticks_to_barline{ ticks_per_measure - j };
			if (ticks_to_barline < voice.at(0).durationData.durationTimeTicks) {
				mx::api::NoteData end_tie_note{ voice.at(0) };
				end_tie_note.durationData = ticks_to_duration_data(voice.at(0).durationData.durationTimeTicks - ticks_to_barline, ticks_per_measure, time_signature);
				if (voice.at(0).isTieStart) {
					end_tie_note.isTieStart = true;
				}
				end_tie_note.isTieStop = true;
				end_tie_note.tickTimePosition = 0;
				voice.insert(voice.begin() + 1, end_tie_note);
				
				voice.at(0).durationData = ticks_to_duration_data(ticks_to_barline, ticks_per_measure, time_signature);
				voice.at(0).isTieStart = true;
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
		std::vector<Voice> follower_voices_list{};
		// LOOP HERE
		int v_shift{ 1 };
		int h_shift{ 3 };
		const Voice follower{ shift(leader, v_shift, h_shift, fifths, ticks_per_measure, time_signature) };
		follower_voices_list.emplace_back(follower);

		// // Create texture
		//const std::vector<Voice> texture{ create_texture() };


		// Create output musicxml
		const mx::api::PartData& original_leader_part{ score.parts.at(0) };
		const mx::api::PartData leader_part{ double_part_length(original_leader_part, original_leader_part.measures.size(), original_leader_part.measures.at(0).timeSignature) };
		
		std::vector<mx::api::PartData> parts_list;
		parts_list.emplace_back(leader_part);
		for (Voice follower_voice : follower_voices_list) {
			parts_list.emplace_back( voice_array_to_part(score, follower, ticks_per_measure, time_signature) ); // Need a function to fix barlines
		}
		const mx::api::ScoreData output_score{ create_output_score(score, parts_list) }; // add follower(s) to original score

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