#include "counterpoint_checker.h"

#include "mx/api/ScoreData.h"

#include <utility>
#include <iostream>
#include <cmath>

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
		//if (current_sonority.get_note_1_motion() == 0 || current_sonority.get_note_2_motion() == 0) {
		//	continue; // Oblique motion is always fine (I think)
		//}
		const Sonority& next_sonority{ (sonority_array).at(i + 1) };

		if (current_sonority.get_note_1().isRest ||
			current_sonority.get_note_2().isRest ||
			next_sonority.get_note_1().isRest ||
			next_sonority.get_note_2().isRest)
		{
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
		if ((i == 0 || (sonority_array.at(i - 1).get_note_1_motion().second != 0 && sonority_array.at(i - 1).get_note_2_motion().second != 0))
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
					&& !downbeat.is_dissonant())
				{

					if (current_sonority.get_simple_interval().second == 7 && downbeat.get_simple_interval().second == 7) {
						send_error_message(Message{ "Parallel fifths between weak beat and downbeat", current_sonority.get_id(), downbeat.get_id() }, error_message_box);
						checked_rhythmic_levels.push_back(downbeat.get_rhythmic_hierarchy());
					} else
					if (current_sonority.get_simple_interval().second == 0 && downbeat.get_simple_interval().second == 0) {
						send_error_message(Message{ "Parallel octaves between weak beat and downbeat", current_sonority.get_id(), downbeat.get_id() }, error_message_box);
						checked_rhythmic_levels.push_back(downbeat.get_rhythmic_hierarchy());
					}

					if (!downbeat.is_dissonant()) {
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
		if (std::abs(current_sonority.get_note_1_motion().first) != 1 &&
			std::abs(current_sonority.get_note_2_motion().first) != 1) {
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
		if (current_sonority.get_motion_type() == similar
			&& current_sonority.get_simple_interval().first == 1
			&& next_sonority.get_simple_interval().first == 2) {
			send_warning_message(Message{ "Similar motion from a 2nd to a 3rd", current_sonority.get_id(), next_sonority.get_id() }, warning_message_box, error_message_box);
		}

	}
}

const bool dissonance_is_allowed(const SonorityArray& sonority_array, const int i) {
	static bool next_sonority_also_legal{ false };
	// There would never be two consecutive sonorities that need to activate next_sonority_also_legal

	if (next_sonority_also_legal) {
		next_sonority_also_legal = false;
		return true;
	}

	// Decide whether to pass these directly
	const Sonority& current_sonority{ sonority_array.at(i) };
	const Sonority& next_sonority{ sonority_array.at(i + 1) };

	if (!next_sonority.is_dissonant()) {
	// If next sonority is consonant

	// Allow appoggiatura. Allow inverted appoggiatura. Allow unaccented appoggiatura. Allow accented passing or neighbor tones.
		if (std::abs((current_sonority.get_note_1_motion().first)) == 1) {
		// If resolves by step
			if ((i != 0)
				|| (sonority_array.at(i - 1).get_note_1().isRest)
				// If preparation is rest
				|| (!sonority_array.at(i - 1).is_dissonant()
					// If preparation is consonant
					&& (sonority_array.at(i - 1).get_note_1_motion().second * current_sonority.get_note_1_motion().second < 0
						// APPOGIATURA: If preparation moves in opposite direction
						|| std::abs(sonority_array.at(i - 1).get_note_1_motion().first) == 1)
					// PASSING/NEIGHBOR NOTE: If preparation moves into the dissonance by step
					)
				)
			{
				return true;
			}
		} else // Only one voice resolves otherwise the current sonority wouldn't be dissonant
		if (std::abs((current_sonority.get_note_1_motion().first)) == 1) {
		// If resolves by step
			if ((i == 0)
				|| (sonority_array.at(i - 1).get_note_2().isRest)
				// If preparation is rest
				|| (!sonority_array.at(i - 1).is_dissonant()
				// If preparation is consonant
					&& (sonority_array.at(i - 1).get_note_2_motion().second * current_sonority.get_note_2_motion().second < 0
					// APPOGIATURA: If preparation moves in opposite direction
						|| std::abs(sonority_array.at(i - 1).get_note_2_motion().first) == 1)
						// PASSING/NEIGHBOR NOTE: If preparation moves into the dissonance by step
					)
				) 
			{

				return true;
			}
		}
		// Else, probably not allowed

	// MAYBE ADD MORE CONDITIONS. (anticipation must be shorter than preparation)
	// Allow unaccented leap to anticipation.
		if ((i != 0)
			&& ((sonority_array.at(i - 1).get_note_1_motion().second == 0 && current_sonority.get_note_2_motion().second == 0)
				|| (sonority_array.at(i - 1).get_note_2_motion().second == 0 && current_sonority.get_note_1_motion().second == 0))
			// If one voice is oblique, then the next
			// ASSUMING other voice cannot move at the same time (research more)
			&& (current_sonority.get_rhythmic_hierarchy() < next_sonority.get_rhythmic_hierarchy())
			// If anticipation is on upbeat
			)
		{
			return true;
		}
	}

	// FIX: SKIP OVER IDENTICAL NOTES OR CONSONANT INTERJECTIONS. SIMILAR FIX AS ANTICIPATIONS  https://en.wikipedia.org/wiki/Cambiata#:~:text=Generally%20it%20refers%20to%20a,the%20opposite%20direction%2C%20and%20where
	// OTHER VOICE CAN'T LEAP??
	// Allow leap to dissonance in cambiata. The cambiata is a figure that usually begins with a downward second to an 
	// unaccented note, then a downward third followed by an upward second to another unaccented note, so that it ends 
	// a third away from its beginning. Both the second and third notes can be dissonant.
		// Upper voice cambiata, current sonority is 2nd note
	if (((i != 0) && (i + 2 < sonority_array.size()))
		// If there are enough notes on either side of the current sonority
		&& (sonority_array.at(i - 1).get_note_1_motion().first == -1)
		&& (current_sonority.get_note_1_motion().first == -2)
		&& (next_sonority.get_note_1_motion().first == 1)
		// If the voice moves correctly
		&& (!sonority_array.at(i - 1).is_dissonant() && !sonority_array.at(i + 2).is_dissonant())
		// If start & end notes are consonant
		&& (current_sonority.get_note_1().durationData.durationTimeTicks <= sonority_array.at(i - 1).get_note_1().durationData.durationTimeTicks && current_sonority.get_note_1().durationData.durationTimeTicks <= next_sonority.get_note_1().durationData.durationTimeTicks)
		// FIX SO IT COUNTS NOTE THAT STARTS IN AN EARLIER SONORITY
		// Second note cannot be longer than notes 1 and 3
		&& (current_sonority.get_rhythmic_hierarchy() <= sonority_array.at(i - 1).get_rhythmic_hierarchy() && current_sonority.get_rhythmic_hierarchy() <= next_sonority.get_rhythmic_hierarchy())
		// Second note must be on weak beat
		)
	{
		//std::cout << "cambiata\n";
		next_sonority_also_legal = true;
		return true;
	} else
		// Lower voice cambiata, current sonority is 2nd note
	if (((i != 0) && (i + 2 < sonority_array.size()))
		// If there are enough notes on either side of the current sonority
		&& (sonority_array.at(i - 1).get_note_2_motion().first == -1)
		&& (current_sonority.get_note_2_motion().first == -2)
		&& (next_sonority.get_note_2_motion().first == 1)
		// If the voice moves correctly
		&& (!sonority_array.at(i - 1).is_dissonant() && !sonority_array.at(i + 2).is_dissonant())
		// If start & end notes are consonant
		&& (current_sonority.get_note_2().durationData.durationTimeTicks <= sonority_array.at(i - 1).get_note_2().durationData.durationTimeTicks && current_sonority.get_note_2().durationData.durationTimeTicks <= next_sonority.get_note_2().durationData.durationTimeTicks)
		// FIX SO IT COUNTS NOTE THAT STARTS IN AN EARLIER SONORITY
		// Second note cannot be longer than notes 1 and 3
		&& (current_sonority.get_rhythmic_hierarchy() <= sonority_array.at(i - 1).get_rhythmic_hierarchy() && current_sonority.get_rhythmic_hierarchy() <= next_sonority.get_rhythmic_hierarchy())
		// Second note must be on weak beat
		)
	{
		//std::cout << "cambiata\n";
		next_sonority_also_legal = true;
		return true;
	} else
	// Upper voice inverted cambiata, current sonority is 2nd note
	if (((i != 0) && (i + 2 < sonority_array.size()))
		// If there are enough notes on either side of the current sonority
		&& (sonority_array.at(i - 1).get_note_1_motion().first == 1)
		&& (current_sonority.get_note_1_motion().first == 2)
		&& (next_sonority.get_note_1_motion().first == -1)
		// If the voice moves correctly
		&& (!sonority_array.at(i - 1).is_dissonant() && !sonority_array.at(i + 2).is_dissonant())
		// If start & end notes are consonant
		&& (current_sonority.get_note_1().durationData.durationTimeTicks <= sonority_array.at(i - 1).get_note_1().durationData.durationTimeTicks && current_sonority.get_note_1().durationData.durationTimeTicks <= next_sonority.get_note_1().durationData.durationTimeTicks)
		// FIX SO IT COUNTS NOTE THAT STARTS IN AN EARLIER SONORITY
		// Second note cannot be longer than notes 1 and 3
		&& (current_sonority.get_rhythmic_hierarchy() <= sonority_array.at(i - 1).get_rhythmic_hierarchy() && current_sonority.get_rhythmic_hierarchy() <= next_sonority.get_rhythmic_hierarchy())
		// Second note must be on weak beat
		)
	{
		//std::cout << "inverted cambiata\n";
		next_sonority_also_legal = true;
		return true;
	} else
		// Lower voice inverted cambiata, current sonority is 2nd note
	if (((i != 0) && (i + 2 < sonority_array.size()))
		// If there are enough notes on either side of the current sonority
		&& (sonority_array.at(i - 1).get_note_2_motion().first == 1)
		&& (current_sonority.get_note_2_motion().first == 2)
		&& (next_sonority.get_note_2_motion().first == -1)
		// If the voice moves correctly
		&& (!sonority_array.at(i - 1).is_dissonant() && !sonority_array.at(i + 2).is_dissonant())
		// If start & end notes are consonant
		&& (current_sonority.get_note_2().durationData.durationTimeTicks <= sonority_array.at(i - 1).get_note_2().durationData.durationTimeTicks && current_sonority.get_note_2().durationData.durationTimeTicks <= next_sonority.get_note_2().durationData.durationTimeTicks)
		// FIX SO IT COUNTS NOTE THAT STARTS IN AN EARLIER SONORITY
		// Second note cannot be longer than notes 1 and 3
		&& (current_sonority.get_rhythmic_hierarchy() <= sonority_array.at(i - 1).get_rhythmic_hierarchy() && current_sonority.get_rhythmic_hierarchy() <= next_sonority.get_rhythmic_hierarchy())
		// Second note must be on weak beat
		)
	{
		//std::cout << "inverted cambiata\n";
		next_sonority_also_legal = true;
		return true;
	}

	next_sonority_also_legal = false;
	return false;
}

void check_dissonance_handling(const SonorityArray& sonority_array, std::vector<Message>& error_message_box, std::vector<Message>& warning_message_box) {
	// TODO: last sonority cannot be dissonant
	for (int i{ 0 }; i < sonority_array.size() - 1; ++i) { // Subtract 1 because we don't want to check the last sonority

		const Sonority& current_sonority{ sonority_array.at(i) };
		const Sonority& next_sonority{ (sonority_array).at(i + 1) };

		if (current_sonority.is_dissonant()) {
			if (!dissonance_is_allowed(sonority_array, i)) {
				send_error_message(Message{ "Illegal dissonance", current_sonority.get_id(), current_sonority.get_id() }, error_message_box);
			}
		}

/*
		// No leap to dissonance
		if (next_sonority.is_dissonant()
			&& (std::abs(current_sonority.get_note_1_motion().first) > 1
				|| std::abs(current_sonority.get_note_2_motion().first) > 1)) {
			send_error_message(Message{ "Leap to dissonance", current_sonority.get_id(), next_sonority.get_id() }, error_message_box);
		}
*/
	}
}

const std::pair<std::vector<Message>, std::vector<Message>>
	check_counterpoint(const std::vector<SonorityArray>& sonority_arrays, const int ticks_per_measure, const int rhythmic_hierarchy_max_depth, const int rhythmic_hierarchy_of_beat) {
	// NOTE: Only use higher rhythmic levels to check for PARALLELS
	// This doesn't care about whether which voice is the bass. It assumes the composer can add another bass voice
	// Return type is a pair of lists of error and warning messages

	std::vector<Message> error_messages{};
	std::vector<Message> warning_messages{};

	for (SonorityArray sonority_array : sonority_arrays) {
		// VOICE INDEPENDENCE
		check_voice_independence(sonority_array, error_messages, warning_messages, ticks_per_measure, rhythmic_hierarchy_max_depth, rhythmic_hierarchy_of_beat);
	}
	check_dissonance_handling(sonority_arrays.at(0), error_messages, warning_messages); // Only check lowest level

	print_messages(error_messages, warning_messages);

	return { error_messages, warning_messages };
}