
#include "exception.h"
#include "settings.h"
#include "file_reader.h"
#include "file_writer.h"
#include "mx/api/ScoreData.h"

#include <iostream>
#include <string>
#include <vector>

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

const Voice shift(Voice voice, int v_shift, int h_shift) { // h_shift in beats. voice is explicitly a copy
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
			//note.pitchData.alter += alters_by_key()
		}
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

const mx::api::PartData voice_array_to_part(const mx::api::ScoreData& score, const Voice& voice) {
	mx::api::PartData part{ score.parts.at(0) };
	part = double_part_length(part, part.measures.size(), part.measures.at(0).timeSignature); // TODO: verify time signature doesn't change
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
		//const int key{}
		const std::string& xml{ read_file(settings::read_file_path) };
		const mx::api::ScoreData& score{ get_score_object(xml) };
		const Voice leader{ create_voice_array(score) };


		// Create follower (LOOP THIS)
		std::vector<Voice> follower_voices_list{};
		// LOOP HERE
		int v_shift{ 1 };
		int h_shift{ 0 };
		const Voice follower{ shift(leader, v_shift, h_shift) };
		follower_voices_list.emplace_back(follower);

		// // Create texture
		//const std::vector<Voice> texture{ create_texture() };


		// Create output musicxml
		const mx::api::PartData& original_leader_part{ score.parts.at(0) };
		const mx::api::PartData leader_part{ double_part_length(original_leader_part, original_leader_part.measures.size(), original_leader_part.measures.at(0).timeSignature) };
		std::vector<mx::api::PartData> parts_list;
		parts_list.emplace_back(leader_part);
		for (Voice follower_voice : follower_voices_list) {
			parts_list.emplace_back( voice_array_to_part(score, follower) ); // Need a function to fix barlines
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