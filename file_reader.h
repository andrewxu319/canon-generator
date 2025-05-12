#pragma once

#include "mx/api/ScoreData.h"
#include <vector>

const std::string read_file(const std::string& path);
const mx::api::ScoreData get_score_object(const std::string& xml);
const std::vector<mx::api::NoteData> create_voice_array(const mx::api::ScoreData& score);