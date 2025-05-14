#include "counterpoint_checker.h"

#include "mx/api/ScoreData.h"

#include <utility>
#include <iostream>

void print_messages(std::vector<Message>& error_message_box, std::vector<Message>& warning_message_box) {
	if (error_message_box.size() == 0 && warning_message_box.size() == 0) {
		std::cout << "No errors or warnings!\n";
	}
	for (Message& error : error_message_box) {
		std::cout << "Error:   Notes at ticks " << std::get<1>(error) << " and " << std::get<2>(error) << ". " << std::get<0>(error) << "!\n";
	}
	for (Message& warning : warning_message_box) {
		std::cout << "Warning: Notes at ticks " << std::get<1>(warning) << " and " << std::get<2>(warning) << ". " << std::get<0>(warning) << "!\n";
	}
}

void send_error_message(const Message& error, std::vector<Message>& error_message_box){
	for (Message& existing_error : error_message_box) {
	// Check if a similar error message already exists
		if ((std::get<1>(existing_error) == std::get<1>(error)) && (std::get<2>(existing_error) == std::get<2>(error))) {
			return;
		}
	}

	error_message_box.emplace_back(error);
}

void send_warning_message(const Message& warning, std::vector<Message>& warning_message_box, std::vector<Message>& error_message_box) {
	for (Message& existing_warning : warning_message_box) {
		// Check if a similar warning OR error message already exists
		if ((std::get<1>(existing_warning) == std::get<1>(warning)) && (std::get<2>(existing_warning) == std::get<2>(warning))) {
			return;
		}
	}
	for (Message& existing_error : error_message_box) {
		if ((std::get<1>(existing_error) == std::get<1>(warning)) && (std::get<2>(existing_error) == std::get<2>(warning))) {
			return;
		}
	}

	warning_message_box.emplace_back(warning);
}

void check_voice_independence(const SonorityArray& sonority_array, std::vector<Message>& error_message_box, std::vector<Message>& warning_message_box, const int ticks_per_measure, const int rhythmic_hierarchy_max_depth, const int rhythmic_hierarchy_of_beat) {
	for (int i{ 0 }; i < sonority_array.size() - 1; ++i) { // Subtract 1 because we don't want to check the last sonority
		const Sonority& current_sonority{ sonority_array.at(i) };
		//if (current_sonority.get_note_1_movement() == 0 || current_sonority.get_note_2_movement() == 0) {
		//	continue; // Oblique motion is always fine (I think)
		//}
		const Sonority& next_sonority{ (sonority_array).at(i + 1) };

		if (current_sonority.get_note_1().isRest ||
			current_sonority.get_note_2().isRest ||
			next_sonority.get_note_1().isRest ||
			next_sonority.get_note_2().isRest) {
			continue;
		}
		
	// Avoid parallel fifths and octaves between adjacent notes.
		if (current_sonority.get_simple_interval().second == 7 && next_sonority.get_simple_interval().second == 7) {
			send_error_message(Message{ "Parallel fifths between adjacent notes or downbeats", current_sonority.get_id(), next_sonority.get_id() } , error_message_box);
		} else
		if (current_sonority.get_simple_interval().second == 0 && next_sonority.get_simple_interval().second == 0) {
			send_error_message(Message{ "Parallel octaves between adjacent notes or downbeats", current_sonority.get_id(), next_sonority.get_id() }, error_message_box);
		}

	// Avoid parallel fifths and octaves between any part of a beat and an accented note on the next beat.
		std::vector<int> checked_rhythmic_levels{}; // Only get FIRST of any given downbeat level
		if ((i == 0 || (sonority_array.at(i - 1).get_note_1_movement().second != 0 && sonority_array.at(i - 1).get_note_2_movement().second != 0))
			// Allow if the first notes don't begin simultaneously
			&& (current_sonority.get_rhythmic_hierarchy() >= rhythmic_hierarchy_of_beat))
			// Allow if both first notes are offbeat
		 {
			for (int j{ i + 1 };
				j < (sonority_array.size())
				&& (sonority_array.at(j).get_id() - current_sonority.get_id() <= ticks_per_measure) // Search up to the end of the measure
				&& (checked_rhythmic_levels.size() < (rhythmic_hierarchy_max_depth - current_sonority.get_rhythmic_hierarchy() - 1)); // End search when all rhythmic levels are found
				++j) {
				const Sonority& downbeat{ sonority_array.at(j) };

				if (downbeat.get_rhythmic_hierarchy() > current_sonority.get_rhythmic_hierarchy()
					// If hierarchy level hasn't been checked yet
					&& find(checked_rhythmic_levels.begin(), checked_rhythmic_levels.end(), downbeat.get_rhythmic_hierarchy()) == checked_rhythmic_levels.end()
					&& is_consonant(downbeat)) {

					if (current_sonority.get_simple_interval().second == 7 && downbeat.get_simple_interval().second == 7) {
						send_error_message(Message{ "Parallel fifths between weak beat and downbeat", current_sonority.get_id(), downbeat.get_id() }, error_message_box);
						checked_rhythmic_levels.push_back(downbeat.get_rhythmic_hierarchy());
					} else
					if (current_sonority.get_simple_interval().second == 0 && downbeat.get_simple_interval().second == 0) {
						send_error_message(Message{ "Parallel octaves between weak beat and downbeat", current_sonority.get_id(), downbeat.get_id() }, error_message_box);
						checked_rhythmic_levels.push_back(downbeat.get_rhythmic_hierarchy());
					}

					if (is_consonant(downbeat)) {
					// Allow if intervening notes (current note is an intervening note for all future notes) are concords.
						break;
					}
				}
			}
		}

	// Avoid parallel fifths and octaves between notes following adjacent accents, if voices are 2:1
		for (int j{ i + 1 }; j < sonority_array.size(); ++j) {
			const Sonority& next_upbeat{ sonority_array.at(j) };
			if (next_upbeat.get_rhythmic_hierarchy() == current_sonority.get_rhythmic_hierarchy()) {
				if (current_sonority.get_simple_interval().second == 7 && next_upbeat.get_simple_interval().second == 7) {
					send_error_message(Message{ "Parallel fifths between consecutive upbeats", current_sonority.get_id(), next_upbeat.get_id() }, error_message_box);
				} else
				if (current_sonority.get_simple_interval().second == 0 && next_upbeat.get_simple_interval().second == 0) {
					send_error_message(Message{ "Parallel octaves between consecutive upbeats", current_sonority.get_id(), next_upbeat.get_id() }, error_message_box);
				}
				break;
			}
		}

/*
	// CHANGE TO WARNING?
	// Avoid leaping motion in two voices moving to a perfect interval. ONLY IF INNER PART
		if (abs(current_sonority.get_note_1_movement().first) != 1 &&
			abs(current_sonority.get_note_2_movement().first) != 1) {
			if (next_sonority.get_simple_interval().second == 7) {
				send_error_message(Message{ "Both voices leap to a perfect fifth", current_sonority.get_id(), next_sonority.get_id() }, error_message_box);
			} else
			if (next_sonority.get_simple_interval().second == 0) {
				send_error_message(Message{ "Both voices leap to an octave", current_sonority.get_id(), next_sonority.get_id() }, error_message_box);
			}
		}
*/

	// Avoid similar motion from a 2nd to a 3rd
	// Remove? Warning?
		if (current_sonority.get_movement_type() == similar
			&& current_sonority.get_simple_interval().first == 1
			&& next_sonority.get_simple_interval().first == 2) {
			send_warning_message(Message{ "Similar motion from a 2nd to a 3rd", current_sonority.get_id(), next_sonority.get_id() }, error_message_box);
		}

	}
}

const std::pair<std::vector<Message>, std::vector<Message>>
	check_counterpoint(const std::vector<SonorityArray>& sonority_arrays, const int ticks_per_measure, const int rhythmic_hierarchy_max_depth, const int rhythmic_hierarchy_of_beat) {
	// NOTE: Only use higher rhythmic levels to check for PARALLELS
	// This doesn't care about whether which voice is the bass. It assumes the composer can add another bass voice
	// Return type is a pair of lists of error and warning messages

	std::vector<Message> error_messages{};
	std::vector<Message> warning_messages{};

	// VOICE INDEPENDENCE
	for (SonorityArray sonority_array : sonority_arrays) {
		check_voice_independence(sonority_array, error_messages, warning_messages, ticks_per_measure, rhythmic_hierarchy_max_depth, rhythmic_hierarchy_of_beat);
	}

	print_messages(error_messages, warning_messages);

	return { error_messages, warning_messages };
}