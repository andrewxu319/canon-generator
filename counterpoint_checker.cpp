#include "counterpoint_checker.h"

#include "mx/api/ScoreData.h"

#include <utility>
#include <iostream>
#include <cmath>

#define SHOW_COUNTERPOINT_CHECKS

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

const int get_note_start_index(const int current_sonority_index, const int voice, const SonorityArray& sonority_array) {
	for (int i{ current_sonority_index - 1 }; i >= 0; --i) {
		if ((sonority_array.at(i).get_note_motion(voice).second != 0) || sonority_array.at(i).get_note(voice).isRest) {
			return i + 1;
			// Search until note isn't stationary or is a rest, then return the next note
		}
	}
	return current_sonority_index; // If no match, current sonority is the result
}

const int get_note_end_index(const int current_sonority_index, const int voice, const SonorityArray& sonority_array) {
	for (int i{ current_sonority_index }; i < sonority_array.size(); ++i) {
		if (sonority_array.at(i).get_note(voice).isRest) {
			return i - 1;
			// If encountering a rest, return the previous note
		}
		if (sonority_array.at(i).get_note_motion(voice).second != 0) {
			return i;
			// Search until note is stationary, then return that note
		}
	}
	return current_sonority_index; // If no match, current sonority is the result
}

const int max_rhythmic_hierarchy(const int first_note_start_sonority_id, const int second_note_sonority_id, const std::vector<int>& rhythmic_hierarchy_array) {
	int max{ 0 };
	for (int i{ first_note_start_sonority_id }; i < second_note_sonority_id; ++i) {
		int max_at_i{ rhythmic_hierarchy_array.at(i % rhythmic_hierarchy_array.size()) };
		if (max_at_i > max) {
			max = max_at_i;
		}
	}
	return max;
}

void is_dissonance_allowed(const SonorityArray& sonority_array, const int i, std::vector<bool>& allowed_dissonances, const int ticks_per_measure, const std::vector<int>& rhythmic_hierarchy_array, std::vector<Message>& error_message_box, std::vector<Message>& warning_message_box) {
	// Decide whether to pass these directly
	const Sonority& current_sonority{ sonority_array.at(i) };
	const Sonority& next_sonority{ sonority_array.at(i + 1) };

	// FIX---ALLOW OTHER VOICE TO MOVE UNDER APPOGIATURA
	// Allow appoggiatura. Allow inverted appoggiatura. Allow unaccented appoggiatura. Allow suspension. Allow accented passing or neighbor tones.
	for (int voice{ 0 }; voice < 2; ++voice) {
		// SUSPENSION
		try {
			if (sonority_array.at(i - 1).get_note_motion(voice).second == 0
				// If preparation is held into the dissonance
				&& (current_sonority.get_rhythmic_hierarchy() > next_sonority.get_rhythmic_hierarchy())
				// The dissonance must occur on a strong beat (1 or 3)
				&& (current_sonority.get_id() - sonority_array.at(get_note_start_index(i - 1, voice, sonority_array)).get_id() >= next_sonority.get_id() - current_sonority.get_id())
				// Require preparation to be equal to or longer than dissonance.
				)
			{
				for (int j{ i + 1 }; j < sonority_array.size(); ++j) {
					const Sonority& resolution{ sonority_array.at(j) };
					if (resolution.get_rhythmic_hierarchy() >= current_sonority.get_rhythmic_hierarchy()) {
						// Stop search upon hitting a stronger or equal beat
						break; // No appogiatura found, go to next check
					}

					// LEGAL ANTICIPATION
					if (get_interval(current_sonority.get_note(voice), resolution.get_note(voice), true).second == 1) {
						// If anticipation that resolves up by semitone (want to only allow leading tone resolutions but can't be bothered
#ifdef SHOW_COUNTERPOINT_CHECKS
						std::cout << "anticipation resolves up by semitone\n";
#endif // SHOW_COUNTERPOINT_CHECKS
						allowed_dissonances.at(i) = true;
						return;
					}

					// ILLEGAL ANTICIPATION
					if (get_interval(current_sonority.get_note(voice), resolution.get_note(voice), true).second > 1) {
						send_error_message(Message{ "Illegal anticipation!", current_sonority.get_id(), resolution.get_id() }, error_message_box);
						allowed_dissonances.at(i) = true; // Not actually allowed but we don't want it to complain again
						return;
					}

					// PROPER DOWNWARD SUSPENSION
					if (get_interval(current_sonority.get_note(voice), resolution.get_note(voice), true).first == -1)
						// Moves by step downwards into resolution
					{
#ifdef SHOW_COUNTERPOINT_CHECKS
						std::cout << "suspension\n";
#endif // SHOW_COUNTERPOINT_CHECKS
						allowed_dissonances.at(i) = true;
						return;
					}
				}
			}
		}
		catch (...) {}

		// APPOGIATURA
		try {
			for (int j{ i + 1 }; j < sonority_array.size(); ++j) {
				const Sonority& resolution{ sonority_array.at(j) };
				if (resolution.get_id() / ticks_per_measure > current_sonority.get_id() / ticks_per_measure) {
					// Stop search at end of measure
					break; // No appogiatura found, go to next check
				}

				if (get_interval(current_sonority.get_note(voice), resolution.get_note(voice), false).first == 1)
					// Moves by step into resolution
				{
					if ((sonority_array.at(i - 1).get_note(voice).isRest)
						// If preparation is rest
						|| (sonority_array.at(i - 1).get_note_motion(voice).second * get_interval(current_sonority.get_note(voice), resolution.get_note(voice), true).second < 0)
						// If preparation moves in opposite direction
						)
					{
						if (current_sonority.get_simple_interval().first == 6 && resolution.get_simple_interval().first == 0) {
							// 7-8 suspensions / appogiaturas are a warning. Comes after passing/neighbor note because those are exempt
							send_warning_message(Message{ "7-8 appogiatura!", current_sonority.get_id(), resolution.get_id() }, warning_message_box, error_message_box);
						}
#ifdef SHOW_COUNTERPOINT_CHECKS
						std::cout << "appogiatura\n";
#endif // SHOW_COUNTERPOINT_CHECKS
						allowed_dissonances.at(i) = true;
						return;
					}
					else {
						// "APPOGIATURA" from a leap of the SAME DIRECTION---warning
						// Preparation to dissonance can't be stationary because that would have been resolved by the suspension check
						send_warning_message(Message{ "Appogiatura leaps in from the same direction!", sonority_array.at(i - 1).get_id(), current_sonority.get_id() }, warning_message_box, error_message_box);
						allowed_dissonances.at(i) = true;
						return;
					}
				}
			}
		}
		catch (...) {}
	}

/*
		if (!next_sonority.is_dissonant()) {
		// Resolve dissonance immediately by step. (Resolution is always the next sonority)

			try {
				if ((std::abs((current_sonority.get_note_motion(voice).first)) == 1)
					// If resolves by step
					&& (!sonority_array.at(i - 1).is_dissonant())) {
					// If preparation is consonant)

					if (std::abs(sonority_array.at(i - 1).get_note_motion(voice).first) == 1) {
						// PASSING/NEIGHBOR NOTE: If preparation moves into the dissonance by step
#ifdef SHOW_COUNTERPOINT_CHECKS
						std::cout << "passing/neighbor note, no delayed resolution\n";
#endif // SHOW_COUNTERPOINT_CHECKS
						allowed_dissonances.at(i) = true;
						return;
					}
					if (current_sonority.get_simple_interval().first == 6 && next_sonority.get_simple_interval().first == 0) {
						// 7-8 suspensions / appogiaturas are a warning. Comes after passing/neighbor note because those are exempt
						send_warning_message(Message{ "7-8 suspension/appogiatura!", current_sonority.get_id(), next_sonority.get_id() }, warning_message_box, error_message_box);
					}
					if (sonority_array.at(i - 1).get_note_motion(voice).second == 0
						// SUSPENSION: If preparation is held into the dissonance
						&& (current_sonority.get_rhythmic_hierarchy() > next_sonority.get_rhythmic_hierarchy())
						// The dissonance must occur on a strong beat (1 or 3)
						&& (current_sonority.get_id() - sonority_array.at(get_note_start_index(i - 1, voice, sonority_array)).get_id() >= next_sonority.get_id() - current_sonority.get_id())
						// Require preparation to be equal to or longer than dissonance.
						)
					{
						if (current_sonority.get_note_motion(voice).second == 1) {
							// If anticipation that resolves up by semitone (want to only allow leading tone resolutions but can't be bothered
#ifdef SHOW_COUNTERPOINT_CHECKS
							std::cout << "anticipation resolves up by semitone\n";
#endif // SHOW_COUNTERPOINT_CHECKS
							allowed_dissonances.at(i) = true;
							return;
						}
						else if (current_sonority.get_note_motion(voice).second > 1) {
							send_error_message(Message{ "Illegal anticipation!", current_sonority.get_id(), next_sonority.get_id() }, error_message_box);
							allowed_dissonances.at(i) = true; // Not actually allowed but we don't want it to complain again
							return;
						}
						else {
							// Proper downward suspension
#ifdef SHOW_COUNTERPOINT_CHECKS
							std::cout << "suspension, no delayed resolution\n";
#endif // SHOW_COUNTERPOINT_CHECKS
							allowed_dissonances.at(i) = true;
							return;
						}
					}
					if ((sonority_array.at(i - 1).get_note(voice).isRest)
						// If preparation is rest
						|| sonority_array.at(i - 1).get_note_motion(voice).second * current_sonority.get_note_motion(voice).second < 0)
					{
						// APPOGIATURA: If preparation moves in opposite direction
#ifdef SHOW_COUNTERPOINT_CHECKS
						std::cout << "appogiatura, no delayed resolution\n";
#endif // SHOW_COUNTERPOINT_CHECKS
						allowed_dissonances.at(i) = true;
						return; // Allowed by default
					}
					else {
						// "APPOGIATURA" from a leap of the SAME DIRECTION---warning
						send_warning_message(Message{ "Appogiatura leaps in from the same direction!", sonority_array.at(i - 1).get_id(), current_sonority.get_id()}, warning_message_box, error_message_box);
						allowed_dissonances.at(i) = true;
						return;
					}
				}
			}
			catch (...) {}
		}
*/

	// MAYBE ADD MORE CONDITIONS. (anticipation must be shorter than preparation, other voice can move during anticipation), Allow successive dissonance if the second tone is an anticipation
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
#ifdef SHOW_COUNTERPOINT_CHECKS
		std::cout << "appogiatura";
#endif // SHOW_COUNTERPOINT_CHECKS
		allowed_dissonances.at(i) = true;
		return;
	}

	// All four-note sequences
	for (int voice{ 0 }; voice < 2; ++voice) {
		try {
			// "Try" ensures there are enough notes on either side
			const int note_2_start{ get_note_start_index(i, voice, sonority_array) };
			const int note_2_end{ get_note_end_index(i, voice, sonority_array) };
			const int note_1_start{ get_note_start_index(note_2_start - 1, voice, sonority_array) };
			const int note_1_end{ note_2_start - 1 };
			const int note_3_start{ note_2_end + 1 };
			const int note_3_end{ get_note_end_index(note_3_start, voice, sonority_array) };
			const int note_4_start{ note_3_end + 1 };
			//const int note_4_end{ get_note_end_index(note_4_start, 1, sonority_array) }; // Unused

/*
* DO THIS LATER BECAUSE WE DON'T CARE ABOUT FOURTHS YET
		// Allow 2 successive dissonances if they are quick and stepwise.
		// Each note is its own sonority because the othe rvoice can't move
			if ((!sonority_array.at(note_1_end).is_dissonant())
				// Note 1 must be consonant
				&& ((sonority_array.at(note_1_end).get_note_motion(voice).first ==  1 && current_sonority.get_note_motion(voice).first ==  1 && next_sonority.get_note_motion(voice).first ==  1) ||
					(sonority_array.at(note_1_end).get_note_motion(voice).first == -1 && current_sonority.get_note_motion(voice).first == -1 && next_sonority.get_note_motion(voice).first == -1))
				// All notes must move stepwise in the same direction
				&& ()
*/

		// FIX: SKIP OVER IDENTICAL NOTES OR CONSONANT INTERJECTIONS. SIMILAR FIX AS ANTICIPATIONS  https://en.wikipedia.org/wiki/Cambiata#:~:text=Generally%20it%20refers%20to%20a,the%20opposite%20direction%2C%20and%20where
		// OTHER VOICE CAN'T LEAP??
		// Allow leap to dissonance in cambiata. Allow double auxiliary.
		// Current sonority = second note
			if ((!sonority_array.at(note_1_end).is_dissonant())
				// Note 1 must be consonant
				&& ((sonority_array.at(note_3_start).get_id() - sonority_array.at(note_2_start).get_id()) <= (sonority_array.at(note_2_start).get_id() - sonority_array.at(note_1_start).get_id()) && (sonority_array.at(note_3_start).get_id() - sonority_array.at(note_2_start).get_id()) <= (sonority_array.at(note_4_start).get_id() - sonority_array.at(note_3_start).get_id()))
				// Second note cannot be longer than notes 1 and 3
				&& (current_sonority.get_rhythmic_hierarchy() <= max_rhythmic_hierarchy(sonority_array.at(note_1_start).get_id(), sonority_array.at(note_2_start).get_id(), rhythmic_hierarchy_array) && current_sonority.get_rhythmic_hierarchy() <= sonority_array.at(note_3_start).get_rhythmic_hierarchy())
				// Second note must be on weak beat
				)
			{
				if ((sonority_array.at(note_1_end).get_note_motion(voice).first == -1)
					&& (sonority_array.at(note_2_end).get_note_motion(voice).first == -2)
					&& (sonority_array.at(note_3_end).get_note_motion(voice).first == 1)) {
					// If the voice moves correctly (downward cambiata)
#ifdef SHOW_COUNTERPOINT_CHECKS
					std::cout << "downward cambiata\n";
#endif // SHOW_COUNTERPOINT_CHECKS
					for (int j{ i }; j <= note_3_end; ++j) {
						allowed_dissonances.at(j) = true;
					}
					return;
				} else
				if ((sonority_array.at(note_1_end).get_note_motion(voice).first == 1)
					&& (sonority_array.at(note_2_end).get_note_motion(voice).first == 2)
					&& (sonority_array.at(note_3_end).get_note_motion(voice).first == -1)) {
					// If the voice moves correctly (inverted upward cambiata)
#ifdef SHOW_COUNTERPOINT_CHECKS
					std::cout << "inverted (upward) cambiata\n";
#endif // SHOW_COUNTERPOINT_CHECKS
					for (int j{ i }; j <= note_3_end; ++j) {
						allowed_dissonances.at(j) = true;
					}
					return;
				} else
				if (((sonority_array.at(note_1_end).get_note_motion(voice).first == -1)
						&& (sonority_array.at(note_2_end).get_note_motion(voice).first == 2)
						&& (sonority_array.at(note_3_end).get_note_motion(voice).first == -1)) ||
						// Down-up double neighbor tone
					((sonority_array.at(note_1_end).get_note_motion(voice).first == 1)
						&& (sonority_array.at(note_2_end).get_note_motion(voice).first == -2)
						&& (sonority_array.at(note_3_end).get_note_motion(voice).first == 1))) {
						// Up-down double neighbor tone
#ifdef SHOW_COUNTERPOINT_CHECKS
					std::cout << "double neighbor tone\n";
#endif // SHOW_COUNTERPOINT_CHECKS
					for (int j{ i }; j <= note_3_end; ++j) {
						allowed_dissonances.at(j) = true;
					}
					return;
				}
			}
		}
		catch (...) {}
	}

// Allow suspension. https://www.youtube.com/watch?v=g-4dw1T3v30
// 7-8 suspension is a warning
	/*
	* Search for preparation
	* Preparation must be consonant
	* IF RESOLUTION NOT DELAYED---make all int
	* Attempt to find resolution 
	*/

// Allow escape tone.
	for (int voice{ 0 }; voice < 2; ++voice) {
		try {
			const int preparation_start{ get_note_start_index(i - 1, voice, sonority_array) };
			const int preparation_end{ i - 1 };
			// I don't think the other voice can move during the escape tone

			if (!sonority_array.at(preparation_end).is_dissonant()
				// End of preparation must be consonant
				&& (current_sonority.get_note(voice).durationData.durationTimeTicks <= current_sonority.get_id() - sonority_array.at(preparation_start).get_id())
				// Preparation must be longer than or equal in length to escape tone
				&& (current_sonority.get_rhythmic_hierarchy() <= sonority_array.at(preparation_start).get_rhythmic_hierarchy() && current_sonority.get_rhythmic_hierarchy() <= next_sonority.get_rhythmic_hierarchy())
				// Preparation must be on weak beat / upbeat
				&& (sonority_array.at(i - 1).get_note_motion(voice).second * current_sonority.get_note_motion(voice).second < 0)
				// Must resolve in opposite direction
				) {
#ifdef SHOW_COUNTERPOINT_CHECKS
				std::cout << "escape tone\n";
#endif // SHOW_COUNTERPOINT_CHECKS
				allowed_dissonances.at(i) = true;
				return;
			}
		}
		catch (...) {}
	}

	// If no rule explicitly says it's legal, use whatever value is already stored
}

void check_dissonance_handling(const SonorityArray& sonority_array, std::vector<Message>& error_message_box, std::vector<Message>& warning_message_box, const int ticks_per_measure, const std::vector<int>& rhythmic_hierarchy_array) {
	// TODO: last sonority cannot be dissonant
	
	std::vector<bool> allowed_dissonances(sonority_array.size());
	// False: not allowed. True: allowed / warning
	
	for (int i{ 0 }; i < sonority_array.size() - 1; ++i) { // Subtract 1 because we don't want to check the last sonority

		const Sonority& current_sonority{ sonority_array.at(i) };
		const Sonority& next_sonority{ (sonority_array).at(i + 1) };

		if (current_sonority.is_dissonant()) {
			is_dissonance_allowed(sonority_array, i, allowed_dissonances, ticks_per_measure, rhythmic_hierarchy_array, error_message_box, warning_message_box);
			// If not allowed, is_dissonance_allowed will mark it as so
		}
		else {
			allowed_dissonances.at(i) = true;
		}
	}

	// Last sonority
	{
		const std::size_t i{ sonority_array.size() - 1 };
		if (sonority_array.at(i).is_dissonant()) {
			allowed_dissonances.at(i) = false;
		}
		else {
			allowed_dissonances.at(i) = true;
		}
	}

	for (int i{ 0 }; i < allowed_dissonances.size(); ++i) {
		if (allowed_dissonances.at(i) == false) {
			send_error_message(Message{ "Illegal dissonance", sonority_array.at(i).get_id(), sonority_array.at(i).get_id() }, error_message_box);
		}
	}
}

const std::pair<std::vector<Message>, std::vector<Message>>
	check_counterpoint(const std::vector<SonorityArray>& sonority_arrays, const int ticks_per_measure, const std::vector<int>& rhythmic_hierarchy_array, const int rhythmic_hierarchy_max_depth, const int rhythmic_hierarchy_of_beat) {
	// NOTE: Only use higher rhythmic levels to check for PARALLELS
	// This doesn't care about whether which voice is the bass. It assumes the composer can add another bass voice
	// Return type is a pair of lists of error and warning messages

	std::vector<Message> error_messages{};
	std::vector<Message> warning_messages{};

	for (SonorityArray sonority_array : sonority_arrays) {
		// VOICE INDEPENDENCE
		check_voice_independence(sonority_array, error_messages, warning_messages, ticks_per_measure, rhythmic_hierarchy_max_depth, rhythmic_hierarchy_of_beat);
	}
	check_dissonance_handling(sonority_arrays.at(0), error_messages, warning_messages, ticks_per_measure, rhythmic_hierarchy_array); // Only check lowest level

	print_messages(error_messages, warning_messages);

	return { error_messages, warning_messages };
}