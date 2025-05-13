#include "exception.h"
#include "settings.h"
#include "mx/api/DocumentManager.h"
#include "mx/api/ScoreData.h"
#include "mx/api/TransposeData.h"

#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <cstdint>
#include <sstream>

const std::string read_file(const std::string& path) { // const
	// ifstream is used for reading files
    std::ifstream file{ path };

	// If we couldn't open the output file stream for reading
	if (!file) {
		// Print an error and exit
        throw Exception("Can't read file \"" + path + "\"!\n");
	}

	// Read file contents to variable https://stackoverflow.com/questions/2602013/read-whole-ascii-file-into-c-stdstring
	auto size = std::filesystem::file_size(path);
	std::string file_contents(size, '\0');
	std::ifstream in(path);
	in.read(&file_contents[0], size);

    //std::cout << file_contents;

	return file_contents;

	// When inf goes out of scope, the ifstream
	// destructor will close the file
}

const mx::api::ScoreData get_score_object(const std::string& xml) {
    // create a reference to the singleton which holds documents in memory for us
    auto& mgr = mx::api::DocumentManager::getInstance();

    // place the xml from above into a stream object
    std::istringstream istr{ xml };

    // ask the document manager to parse the xml into memory for us, returns a document ID.
    const auto documentID = mgr.createFromStream(istr);

    // get the structural representation of the score from the document manager
    const auto score = mgr.getData(documentID);

    // we need to explicitly destroy the document from memory
    mgr.destroyDocument(documentID);

    return score;
}

const std::vector<mx::api::NoteData> create_voice_array(const mx::api::ScoreData& score) {
    if (score.parts.size() != 1)
    {
        throw Exception("More than one part in source!");
    }

    const mx::api::PartData part{ score.parts.at(0)};

    std::vector<std::vector<mx::api::NoteData>> array_of_condensed_measures{}; // Array of measures, each with only sequence of notes inside
    array_of_condensed_measures.reserve(sizeof(part));
    for (const mx::api::MeasureData& measure : part.measures) {
        if (measure.staves.size() < 1) {
            throw Exception("Zero staves found!");
        }
        else if (measure.staves.size() > 1) {
            std::cout << "More than one staff found. Using first one.";
        }

        const mx::api::StaffData& staff{ measure.staves.at(0) };
        if (staff.voices.size() != 1) {
            throw Exception("Part doesn't have exactly one voice!");
        }

        std::vector<mx::api::NoteData> measure_condensed{ staff.voices.at(0).notes };

        array_of_condensed_measures.push_back(measure_condensed);
    }

    // Condense array_of_condensed_measures
    std::vector<mx::api::NoteData> part_condensed{};
    part_condensed.reserve(sizeof(part));
    for (std::vector<mx::api::NoteData> condensed_measure : array_of_condensed_measures) {
        for (mx::api::NoteData note : condensed_measure) {
            part_condensed.push_back(note);
        }
    }

    return part_condensed; // WORK ON THIS
}