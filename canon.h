#pragma once

#include "mx/api/ScoreData.h"

#include <vector>

using Voice = std::vector<mx::api::NoteData>;

class Message {
public:
	const std::string_view description{};
	const int sonority_1_index{};
	const int sonority_2_index{};
};

class Canon {
public:
	Canon(std::vector<Voice> texture, int max_h_shift)
		: m_texture{ texture }, m_max_h_shift{ max_h_shift } {
	}

	std::vector<Voice>& texture() {
		return m_texture;
	}

	const int get_max_h_shift() const {
		return m_max_h_shift;
	}

	/*std::vector<Message> error_message_box() const {
		return m_error_message_box;
	}

	std::vector<Message> warning_message_box() const {
		return m_warning_message_box;
	}

	const int get_warning_count() const {
		return m_warning_message_box.size();
	}*/

private:
	std::vector<Voice> m_texture{};
	int m_max_h_shift{};
	/*std::vector<Message> m_error_message_box{};
	std::vector<Message> m_warning_message_box{};*/
	int m_invertibility{};
	int m_top_voice{}; // smth smth fix
	int m_bottom_voice{}; // fix
	int m_versatility{};
};