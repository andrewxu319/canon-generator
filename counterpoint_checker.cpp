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
		std::cout << "Error:   Notes at ticks " << (error.sonority_1_index) << " and " << (error.sonority_2_index) << ". " << (error.description) << "!\n";
	}
	for (Message& warning : warning_message_box) {
		std::cout << "Warning: Notes at ticks " << (warning.sonority_1_index) << " and " << (warning.sonority_2_index) << ". " << (warning.description) << "!\n";
	}
}

void send_error_message(const Message& error, std::vector<Message>& error_message_box){
	for (Message& existing_error : error_message_box) {
	// Check if a similar error message already exists
		if (((existing_error.sonority_1_index) == (error.sonority_1_index)) && ((existing_error.sonority_2_index) == (error.sonority_2_index))) {
			return;
		}
	}

	error_message_box.emplace_back(error);
}

void send_warning_message(const Message& warning, std::vector<Message>& warning_message_box, std::vector<Message>& error_message_box) {
	for (Message& existing_warning : warning_message_box) {
		// Check if a similar warning OR error message already exists
		if (((existing_warning.sonority_1_index) == (warning.sonority_1_index)) && ((existing_warning.sonority_2_index) == (warning.sonority_2_index))) {
			return;
		}
	}
	for (Message& existing_error : error_message_box) {
		if (((existing_error.sonority_1_index) == (warning.sonority_1_index)) && ((existing_error.sonority_2_index) == (warning.sonority_2_index))) {
			return;
		}
	}

	warning_message_box.emplace_back(warning);
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

const bool are_upbeat_parallels_legal(const SonorityArray& sonority_array, const std::vector<int>& index_array, const Sonority& current_sonority, const int note_1_index, const int note_2_index) {
	// Allow if there is a consonant suspension between the parallels
	{
		for (int voice{ 0 }; voice < 2; ++voice) {
/* Not actually sure if this is a rule
			if (sonority_array.at(get_note_start_index(note_1_index, voice, sonority_array)).get_rhythmic_hierarchy() > sonority_array.at(note_2_index).get_rhythmic_hierarchy()) {
				// If note 1 starts on a strong(er) beat, consonant suspension doesn't make the parallels legal
				continue;
			}
*/

			int consonant_suspension_start{ -1 };
			for (int k{ note_1_index + 1 }; k < note_2_index; ++k) {
				// Find intervening downbeat
				if (sonority_array.at(index_array.at(k)).get_rhythmic_hierarchy() > current_sonority.get_rhythmic_hierarchy()) {
					consonant_suspension_start = index_array.at(k);
				}
			}
			if (consonant_suspension_start == -1) {
				// No valid downbeat found. Terminate loop for both voices.
				break;
			}

			const int consonant_suspension_end{ get_note_end_index(consonant_suspension_start, 0, sonority_array) };
			if (sonority_array.at(consonant_suspension_start).is_dissonant() || sonority_array.at(consonant_suspension_end).is_dissonant()) {
				// If consonant suspension isn't consonant, terminate loop for current voice
				continue;
			}
			bool note_1_is_held{ true };
			for (int k{ index_array.at(note_1_index) }; k < consonant_suspension_end; ++k) {
				// Loop through all notes between current_sonority and the the end of the consonant suspension to make sure the suspension is held thorugh
				if (sonority_array.at(k).get_note_motion(voice).second != 0) {
					note_1_is_held = false;
				}
			}
			if (!note_1_is_held) {
				continue;
			}
#ifdef SHOW_COUNTERPOINT_CHECKS
			std::cout << "consonant suspension legalizes perfect parallels\n";
#endif // SHOW_COUNTERPOINT_CHECKS
			return true;
		}
	}

	// Allow if each of the notes forming the parallel is approached from a different direction.
	for (int voice{ 0 }; voice < 2; ++voice) {
		try {
			if ((sonority_array.at(index_array.at(note_1_index) - 1).get_note_motion(voice).second * sonority_array.at(index_array.at(note_2_index) - 1).get_note_motion(voice).second < 0)
				|| (get_interval(sonority_array.at(index_array.at(note_1_index - 1)).get_note(voice), sonority_array.at(index_array.at(note_1_index)).get_note(voice), true).second
					* get_interval(sonority_array.at(index_array.at(note_2_index - 1)).get_note(voice), sonority_array.at(index_array.at(note_2_index)).get_note(voice), true).second
					< 0)
				// First line checks each voice, checks whether the notes before the perfect parallels (regardless of rhythmic hierarchy) approach from opposite directions
				// Other lines do the same but for notes of equal or greater rhythmic hierarchy immediately before the sonorities comprising the perfect intervals
				)
			{
				return true;
			}
		}
		catch(...){}
	}

	return false;
}

void check_voice_independence(const SonorityArray& sonority_array, std::vector<int> index_array, std::vector<Message>& error_message_box, std::vector<Message>& warning_message_box, const int ticks_per_measure, const int rhythmic_hierarchy_max_depth, const int rhythmic_hierarchy_of_beat) {
	// Check if inverting fixes the parallels
	bool parallel_fifths { false };
	bool parallel_fourths{ false };
	int parallel_fifths_start{};
	int parallel_fifths_end{};
	
	for (int i{ 0 }; i < index_array.size() - 1; ++i) { // Subtract 1 because we don't want to check the last sonority
		const Sonority& current_sonority{ sonority_array.at(index_array.at(i)) };
		const Sonority& next_sonority{ (sonority_array).at(index_array.at(i + 1)) };

		if (current_sonority.get_note_1().isRest ||
			current_sonority.get_note_2().isRest ||
			next_sonority.get_note_1().isRest ||
			next_sonority.get_note_2().isRest)
		{
			continue;
		}
		
	// Avoid parallel fifths and octaves between adjacent notes.
		if (current_sonority.get_simple_interval().second == 7 && next_sonority.get_simple_interval().second == 7) {
			parallel_fifths = true;
			parallel_fifths_start = current_sonority.get_index();
			parallel_fifths_end = next_sonority.get_index();
#ifdef SHOW_COUNTERPOINT_CHECKS
			std::cout << "Parallel fifths between adjacent notes or downbeats, checking inverted version\n";
#endif // SHOW_COUNTERPOINT_CHECKS
			//send_error_message(Message{ "Parallel fifths between adjacent notes or downbeats", current_sonority.get_index(), next_sonority.get_index() } , error_message_box);
		} else
		if (current_sonority.get_simple_interval().second == 5 && next_sonority.get_simple_interval().second == 5) {
			parallel_fourths = true;
#ifdef SHOW_COUNTERPOINT_CHECKS
			std::cout << "Parallel fourths between adjacent notes or downbeats, checking inverted version\n";
#endif // SHOW_COUNTERPOINT_CHECKS
		} else
		if (current_sonority.get_simple_interval().first == 0 && next_sonority.get_simple_interval().first == 0) {
			send_error_message(Message{ "Parallel octaves between adjacent notes or downbeats", current_sonority.get_index(), next_sonority.get_index() }, error_message_box);
		}

	// Avoid parallel fifths and octaves between any part of a beat and an accented note on the next beat.
		std::vector<int> checked_rhythmic_levels{}; // Only get FIRST of any given downbeat level
		if ((i == 0 || (sonority_array.at(index_array.at(i - 1)).get_note_1_motion().second != 0 && sonority_array.at(index_array.at(i - 1)).get_note_2_motion().second != 0))
			// Allow if the first notes don't begin simultaneously
			&& (current_sonority.get_rhythmic_hierarchy() >= rhythmic_hierarchy_of_beat))
			// Allow if both first notes are offbeat
		 {
			for (int j{ i + 1 };
				j < (index_array.size())
				&& (sonority_array.at(index_array.at(j)).get_index() - current_sonority.get_index() <= ticks_per_measure) // Search up to the end of the measure
				&& (checked_rhythmic_levels.size() < (rhythmic_hierarchy_max_depth - current_sonority.get_rhythmic_hierarchy() - 1)); // End search when all rhythmic levels are found
				++j) {
				const Sonority& downbeat{ sonority_array.at(index_array.at(j)) };

				if (downbeat.get_rhythmic_hierarchy() > current_sonority.get_rhythmic_hierarchy()
					// If hierarchy level hasn't been checked yet
					&& find(checked_rhythmic_levels.begin(), checked_rhythmic_levels.end(), downbeat.get_rhythmic_hierarchy()) == checked_rhythmic_levels.end()
					&& !downbeat.is_dissonant())
				{

					if (current_sonority.get_simple_interval().second == 7 && downbeat.get_simple_interval().second == 7) {
						parallel_fifths = true;
						parallel_fifths_start = current_sonority.get_index();
						parallel_fifths_end = downbeat.get_index();
#ifdef SHOW_COUNTERPOINT_CHECKS
						std::cout << "Parallel fifths between weak beat and downbeat, checking inverted version\n";
#endif // SHOW_COUNTERPOINT_CHECKS
						//send_error_message(Message{ "Parallel fifths between weak beat and downbeat", current_sonority.get_index(), downbeat.get_index() }, error_message_box);
						checked_rhythmic_levels.push_back(downbeat.get_rhythmic_hierarchy());
					} else
					if (current_sonority.get_simple_interval().second == 7 && downbeat.get_simple_interval().second == 7) {
						parallel_fourths = true;
#ifdef SHOW_COUNTERPOINT_CHECKS
						std::cout << "Parallel fourths between weak beat and downbeat, checking inverted version\n";
#endif // SHOW_COUNTERPOINT_CHECKS
						checked_rhythmic_levels.push_back(downbeat.get_rhythmic_hierarchy());
					}
					else
					if (current_sonority.get_simple_interval().first == 0 && downbeat.get_simple_interval().first == 0) {
						send_error_message(Message{ "Parallel octaves between weak beat and downbeat", current_sonority.get_index(), downbeat.get_index() }, error_message_box);
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
		for (int j{ i + 1 }; j < index_array.size(); ++j) {
			const Sonority& next_upbeat{ sonority_array.at(index_array.at(j)) };
			if (next_upbeat.get_rhythmic_hierarchy() == current_sonority.get_rhythmic_hierarchy()) {
				if (current_sonority.get_simple_interval().second == 7 && next_upbeat.get_simple_interval().second == 7) {
					if (!are_upbeat_parallels_legal(sonority_array, index_array, current_sonority, i, j)) {
						parallel_fifths = true;
						parallel_fifths_start = current_sonority.get_index();
						parallel_fifths_end = next_upbeat.get_index();
#ifdef SHOW_COUNTERPOINT_CHECKS
						std::cout << "Parallel fifths between consecutive upbeats, checking inverted version";
#endif // SHOW_COUNTERPOINT_CHECKS
						//send_error_message(Message{ "Parallel fifths between consecutive upbeats", current_sonority.get_index(), next_upbeat.get_index() }, error_message_box);
					}
				} else
				if (current_sonority.get_simple_interval().second == 7 && next_upbeat.get_simple_interval().second == 7) {
					if (!are_upbeat_parallels_legal(sonority_array, index_array, current_sonority, i, j)) {
						parallel_fourths = true;
#ifdef SHOW_COUNTERPOINT_CHECKS
						std::cout << "Parallel fourths between consecutive upbeats, checking inverted version";
#endif // SHOW_COUNTERPOINT_CHECKS
						//send_error_message(Message{ "Parallel fifths between consecutive upbeats", current_sonority.get_index(), next_upbeat.get_index() }, error_message_box);
					}
				} else
				if (current_sonority.get_simple_interval().first == 0 && next_upbeat.get_simple_interval().first == 0) {
					if (!are_upbeat_parallels_legal(sonority_array, index_array, current_sonority, i, j)) {
						send_error_message(Message{ "Parallel octaves between consecutive upbeats", current_sonority.get_index(), next_upbeat.get_index() }, error_message_box);
					}
				}
				break;
			}
		}

		if (parallel_fifths || parallel_fourths) {
			send_error_message(Message{ "Parallel fifths!", parallel_fifths_start, parallel_fifths_end }, error_message_box);
		}

/*
	// CHANGE TO WARNING?
	// Avoid leaping motion in two voices moving to a perfect interval. ONLY IF INNER PART
		if (std::abs(current_sonority.get_note_1_motion().first) != 1 &&
			std::abs(current_sonority.get_note_2_motion().first) != 1) {
			if (next_sonority.get_simple_interval().second == 7) {
				send_error_message(Message{ "Both voices leap to a perfect fifth", current_sonority.get_index(), next_sonority.get_index() }, error_message_box);
			} else
			if (next_sonority.get_simple_interval().second == 0) {
				send_error_message(Message{ "Both voices leap to an octave", current_sonority.get_index(), next_sonority.get_index() }, error_message_box);
			}
		}
*/

	// Avoid similar motion from a 2nd to a 3rd
	// Remove? Warning?
		if (current_sonority.get_motion_type() == similar
			&& (current_sonority.get_simple_interval().first == 1
				&& next_sonority.get_simple_interval().first == 2)
			// 2nd to 3rd
			|| (current_sonority.get_simple_interval().first == 6
				&& next_sonority.get_simple_interval().first == 5))
			// Inverted---7th to 6th
		{
			send_warning_message(Message{ "Similar motion from a 2nd to a 3rd", current_sonority.get_index(), next_sonority.get_index() }, warning_message_box, error_message_box);
		}

	}
}

const int max_rhythmic_hierarchy(const int first_note_start_sonority_index, const int second_note_sonority_index, const std::vector<int>& rhythmic_hierarchy_array) {
	int max{ 0 };
	for (int i{ first_note_start_sonority_index }; i < second_note_sonority_index; ++i) {
		int max_at_i{ rhythmic_hierarchy_array.at(i % rhythmic_hierarchy_array.size()) };
		if (max_at_i > max) {
			max = max_at_i;
		}
	}
	return max;
}

void is_dissonance_allowed(const SonorityArray& sonority_array, const int i, std::vector<bool>& allowed_dissonances, const int ticks_per_measure, const ScaleDegree& tonic, const ScaleDegree& dominant, const ScaleDegree& leading_tone, const std::vector<int>& rhythmic_hierarchy_array, std::vector<Message>& error_message_box, std::vector<Message>& warning_message_box) {
	// Decide whether to pass these directly
	const Sonority& current_sonority{ sonority_array.at(i) };
	const Sonority& next_sonority{ sonority_array.at(i + 1) };

	// PEDAL TONE
	for (int voice{ 0 }; voice < 2; ++voice) {
		try {
			const int pedal_start{ get_note_start_index(i, voice, sonority_array) };
			const int pedal_end{ get_note_end_index(i, voice, sonority_array) };

			if ((pedal_end - pedal_start >= 6)
				// A pedal will be defined as a note that lasts (or repeats) through more than six pitch changes in other voices. Really it would be better to define it as a tone that is held through two or more changes of harmony, but for technical reasons we'll make this arbitrary definition.
				&& ((sonority_array.at(pedal_start).get_note(voice).pitchData.step == tonic.step)
					// Pedal can be tonic
					|| (sonority_array.at(pedal_start).get_note(voice).pitchData.step == dominant.step))
				// Or dominant
				&& (!sonority_array.at(pedal_start).is_dissonant() && !sonority_array.at(pedal_end).is_dissonant())
				// Start and end must be consonant
				)
			{
				for (int j{ pedal_start }; j <= pedal_end; ++j) {
					allowed_dissonances.at(j) = true;
				}
#ifdef SHOW_COUNTERPOINT_CHECKS
				std::cout << "pedal tone\n";
#endif // SHOW_COUNTERPOINT_CHECKS
				return;
			}
		}
		catch (...) {}
	}

	for (int voice{ 0 }; voice < 2; ++voice) {
		// Allow 2 successive dissonances if they are quick and stepwise.
		// Here we define "quick" as there are no other voices moving faster
		try {
			if ((!sonority_array.at(i - 1).is_dissonant())
				// Note 1 must be consonant
				&& (!sonority_array.at(i + 2).is_dissonant())
				// Note 4 must be consonant
				)
			{
				if ((sonority_array.at(i - 1).get_note_motion(voice).first == 1 && current_sonority.get_note_motion(voice).first == 1 && next_sonority.get_note_motion(voice).first == 1) ||
					(sonority_array.at(i - 1).get_note_motion(voice).first == -1 && current_sonority.get_note_motion(voice).first == -1 && next_sonority.get_note_motion(voice).first == -1))
					// All notes must move stepwise in the same direction
				{
#ifdef SHOW_COUNTERPOINT_CHECKS
					std::cout << "double passing tones\n";
#endif // SHOW_COUNTERPOINT_CHECKS
					allowed_dissonances.at(i) = true;
					allowed_dissonances.at(i + 1) = true;
					return;
				}
				if ((sonority_array.at(i - 1).get_note_motion(voice).first == -1)
					// Note 1 descends stepwise
					&& (current_sonority.get_note_motion(voice).first == -1)
					// Note 2 descends stepwise
					&& (next_sonority.get_note_motion(voice).first == 0)
					// Note 3 stays stationary
					&& (sonority_array.at(i + 2).get_rhythmic_hierarchy() > next_sonority.get_rhythmic_hierarchy())
					// Note 3 must be on upbeat
					&& ((sonority_array.at(i + 2).get_index() - next_sonority.get_index()) <= (next_sonority.get_index() - current_sonority.get_index()))
					// Note 3 cannot be longer than note 2
					)
				{
#ifdef SHOW_COUNTERPOINT_CHECKS
					std::cout << "anticipation with two dissonances\n";
#endif // SHOW_COUNTERPOINT_CHECKS
					allowed_dissonances.at(i) = true;
					allowed_dissonances.at(i + 1) = true;
					return;
				}
			}
		}
		catch (...) {}
	}

	// CAMBIATA, DOUBLE NEIGHBOR NOTE (and all other four-note sequences)
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

// FIX: SKIP OVER IDENTICAL NOTES OR CONSONANT INTERJECTIONS. SIMILAR FIX AS ANTICIPATIONS  https://en.wikipedia.org/wiki/Cambiata#:~:text=Generally%20it%20refers%20to%20a,the%20opposite%20direction%2C%20and%20where
// OTHER VOICE CAN'T LEAP??
// Allow leap to dissonance in cambiata. Allow double auxiliary.
// Current sonority = second note
			if ((!sonority_array.at(note_1_end).is_dissonant())
				// Note 1 must be consonant
				&& ((sonority_array.at(note_3_start).get_index() - sonority_array.at(note_2_start).get_index()) <= (sonority_array.at(note_2_start).get_index() - sonority_array.at(note_1_start).get_index()) && (sonority_array.at(note_3_start).get_index() - sonority_array.at(note_2_start).get_index()) <= (sonority_array.at(note_4_start).get_index() - sonority_array.at(note_3_start).get_index()))
				// Second note cannot be longer than notes 1 and 3
				&& (current_sonority.get_rhythmic_hierarchy() <= max_rhythmic_hierarchy(sonority_array.at(note_1_start).get_index(), sonority_array.at(note_2_start).get_index(), rhythmic_hierarchy_array) && current_sonority.get_rhythmic_hierarchy() <= sonority_array.at(note_3_start).get_rhythmic_hierarchy())
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
				}
				else
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
					}
					else
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

	// Here we've dealt with all multi-dissonance sequences. Prohibit all other consecutive dissonances
	if (next_sonority.is_dissonant()) {
#ifdef SHOW_COUNTERPOINT_CHECKS
		std::cout << "multiple consecutive dissonances error\n";
#endif // SHOW_COUNTERPOINT_CHECKS
		allowed_dissonances.at(i) = true;
		return;
	}

	for (int voice{ 0 }; voice < 2; ++voice) {
		// PASSING & NEIGHBOR NOTE
		try {
			if ((!next_sonority.is_dissonant())
				// Resolve dissonance immediately by step. (Resolution is always the next sonority)
				&& (std::abs(sonority_array.at(i - 1).get_note_motion(voice).first) == 1)
				// If prepared by step
				&& (std::abs((current_sonority.get_note_motion(voice).first)) == 1)
				// If resolves by step
				&& (!sonority_array.at(i - 1).is_dissonant())
				// If preparation is consonant
				)
			{
#ifdef SHOW_COUNTERPOINT_CHECKS
				std::cout << "passing/neighbor note\n";
#endif // SHOW_COUNTERPOINT_CHECKS
				allowed_dissonances.at(i) = true;
				return;
			}
		}
		catch (...) {}

		// SUSPENSION
		try {
			if (sonority_array.at(i - 1).get_note_motion(voice).second == 0
				// If preparation is held into the dissonance
				&& (current_sonority.get_rhythmic_hierarchy() > next_sonority.get_rhythmic_hierarchy())
				// The dissonance must occur on a strong beat (1 or 3)
				&& (current_sonority.get_index() - sonority_array.at(get_note_start_index(i - 1, voice, sonority_array)).get_index() >= next_sonority.get_index() - current_sonority.get_index())
				// Require preparation to be equal to or longer than dissonance.
				)
			{
				for (int j{ i + 1 }; j < sonority_array.size(); ++j) {
					const Sonority& resolution{ sonority_array.at(j) };
					if (resolution.get_rhythmic_hierarchy() >= current_sonority.get_rhythmic_hierarchy()) {
						// Stop search upon hitting a stronger or equal beat
						break; // No appogiatura found, go to next check
					}

					// LEGAL RETARDATION
					if (current_sonority.get_note(voice).pitchData.step == leading_tone.step && current_sonority.get_note(voice).pitchData.alter == leading_tone.alter
						&& resolution.get_note(voice).pitchData.step == tonic.step && resolution.get_note(voice).pitchData.alter == tonic.alter) {
						// If retardation that resolves up by semitone (want to only allow leading tone resolutions but can't be bothered
#ifdef SHOW_COUNTERPOINT_CHECKS
						std::cout << "retardation resolves up by semitone\n";
#endif // SHOW_COUNTERPOINT_CHECKS
						allowed_dissonances.at(i) = true;
						return;
					}

/* commenting this out because we want to check all other conditions
					// ILLEGAL RETARDATION
					if (get_interval(current_sonority.get_note(voice), resolution.get_note(voice), true).second > 1) {
						send_error_message(Message{ "Illegal retardation!", current_sonority.get_index(), resolution.get_index() }, error_message_box);
						allowed_dissonances.at(i) = true; // Not actually allowed but we don't want it to complain again
						return;
					}
*/

					// PROPER DOWNWARD SUSPENSION
					if (get_interval(current_sonority.get_note(voice), resolution.get_note(voice), true).first == -1)
						// Moves by step downwards into resolution
					{
						if (resolution.get_simple_interval().first == 0 || resolution.get_simple_interval().second == 7) {
							send_warning_message(Message{ "Suspension resolves to perfect consonance", current_sonority.get_index(), resolution.get_index() }, warning_message_box, error_message_box);
						}

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
				if (resolution.get_index() / ticks_per_measure > current_sonority.get_index() / ticks_per_measure) {
					// Stop search at end of measure
					break; // No appogiatura found, go to next check
				}

				const int other_voice{ (voice == 0) ? 1 : 0 };
				if ((get_interval(current_sonority.get_note(voice), resolution.get_note(voice), false).first == 1)
					// Moves by step into resolution
					&& (get_interval(current_sonority.get_note(other_voice), resolution.get_note(other_voice), true).second == 0)
					// Other voice must be the same at resolution
					)
				{
					if ((sonority_array.at(i - 1).get_note(voice).isRest)
						// If preparation is rest
						|| (sonority_array.at(i - 1).get_note_motion(voice).second * get_interval(current_sonority.get_note(voice), resolution.get_note(voice), true).second < 0)
						// If preparation leaps in opposite direction
						)
					{
						if (resolution.get_simple_interval().first == 0) {
							// Can't really resolve to a perfect 5th since other voice can't move
							send_warning_message(Message{ "Appogiatura resolution is doubled!", current_sonority.get_index(), resolution.get_index() }, warning_message_box, error_message_box);
						}
#ifdef SHOW_COUNTERPOINT_CHECKS
						std::cout << "appogiatura\n";
#endif // SHOW_COUNTERPOINT_CHECKS
						allowed_dissonances.at(i) = true;
						return;
					}
					else if (std::abs(sonority_array.at(i - 1).get_note_motion(voice).second > 1)) {
						// "APPOGIATURA" from a leap of the SAME DIRECTION---warning
						// Preparation to dissonance can't be stationary because that would have been resolved by the suspension check
						
						//if (allowed_dissonances.at(i) == false) {
						//	// Ensures something marked as the third note of a cambiata/double neighbor tone sequence isn't checked 
							send_warning_message(Message{ "Appogiatura leaps in from the same direction!", sonority_array.at(i - 1).get_index(), current_sonority.get_index() }, warning_message_box, error_message_box);
						//}
						allowed_dissonances.at(i) = true;
						return;
					}
				}
			}
		}
		catch (...) {}
	}

	// MAYBE ADD MORE CONDITIONS. (anticipation must be shorter than preparation, other voice can move during anticipation), Allow successive dissonance if the second tone is an anticipation
	// Allow unaccented leap to anticipation.
	for (int voice{ 0 }; voice < 2; ++voice) {
		const int other_voice{ (voice == 0) ? 1 : 0 };
		const int preparation_start{ get_note_start_index(i - 1, voice, sonority_array) };
		try {
			if ((sonority_array.at(i - 1).get_note_motion(other_voice).second == 0 && current_sonority.get_note_motion(voice).second == 0)
				// If one voice is oblique, then the next
				// Other voice can't move during anticipation. In multi-voice textures, one voice might have an anticipation in relation to another slower-moving voice, but have non-anticipation counterpoint in relation to another faster-moving voice
				&& (sonority_array.at(i - 1).get_note_motion(voice).second < 0)
				// Always uses descending motion, never ascending motion
				&& (current_sonority.get_rhythmic_hierarchy() < next_sonority.get_rhythmic_hierarchy() && current_sonority.get_rhythmic_hierarchy() <= sonority_array.at(preparation_start).get_rhythmic_hierarchy())
				// If dissonance is on upbeat with relation to the preparation and at least the same hierarchy as the resolution
				&& ((next_sonority.get_index() - current_sonority.get_index()) <= (current_sonority.get_index() - sonority_array.at(preparation_start).get_index()))
				// Preparation must be at least the same length as the dissonance
				)
			{
#ifdef SHOW_COUNTERPOINT_CHECKS
				std::cout << "anticipation\n";
#endif // SHOW_COUNTERPOINT_CHECKS
				allowed_dissonances.at(i) = true;
				return;
			}
		}
		catch (...){}
	}

// Allow suspension. https://www.youtube.com/watch?v=g-4dw1T3v30
// 7-8 suspension is a warning
	/*
	* Search for preparation
	* Preparation must be consonant
	* IF RESOLUTION NOT DELAYED---make all int
	* Attempt to find resolution 
	*/

// ESCAPE TONE
	for (int voice{ 0 }; voice < 2; ++voice) {
		try {
			const int preparation_start{ get_note_start_index(i - 1, voice, sonority_array) };
			const int preparation_end{ i - 1 };
			// I don't think the other voice can move during the escape tone

			if (!sonority_array.at(preparation_end).is_dissonant()
				// End of preparation must be consonant
				&& (current_sonority.get_note(voice).durationData.durationTimeTicks <= current_sonority.get_index() - sonority_array.at(preparation_start).get_index())
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

void check_dissonance_handling(const SonorityArray& sonority_array, std::vector<Message>& error_message_box, std::vector<Message>& warning_message_box, const int ticks_per_measure, const ScaleDegree& tonic, const ScaleDegree& dominant, const ScaleDegree& leading_tone, const std::vector<int>& rhythmic_hierarchy_array) {
	// TODO: last sonority cannot be dissonant
	
	std::vector<bool> allowed_dissonances(sonority_array.size());
	// False: not allowed. True: allowed / warning
	
	for (int i{ 0 }; i < sonority_array.size() - 1; ++i) { // Subtract 1 because we don't want to check the last sonority

		const Sonority& current_sonority{ sonority_array.at(i) };
		const Sonority& next_sonority{ (sonority_array).at(i + 1) };

		if (current_sonority.is_dissonant()) {
			is_dissonance_allowed(sonority_array, i, allowed_dissonances, ticks_per_measure, tonic, dominant, leading_tone, rhythmic_hierarchy_array, error_message_box, warning_message_box);
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
			send_error_message(Message{ "Illegal dissonance", sonority_array.at(i).get_index(), sonority_array.at(i).get_index() }, error_message_box);
		}
	}
}

const std::pair<std::vector<Message>, std::vector<Message>>
	check_counterpoint(const SonorityArray& sonority_array, const std::vector<std::vector<int>> index_arrays_for_sonority_arrays, const int ticks_per_measure, const ScaleDegree& tonic, const ScaleDegree& dominant, const ScaleDegree& leading_tone, const std::vector<int>& rhythmic_hierarchy_array, const int rhythmic_hierarchy_max_depth, const int rhythmic_hierarchy_of_beat) {
	// NOTE: Only use higher rhythmic levels to check for PARALLELS
	// This doesn't care about whether which voice is the bass. It assumes the composer can add another bass voice
	// Return type is a pair of lists of error and warning messages

	std::vector<Message> error_messages{};
	std::vector<Message> warning_messages{};

	for (std::vector<int> index_array : index_arrays_for_sonority_arrays) {
		if (index_array.size() > 0) {
			check_voice_independence(sonority_array, index_array, error_messages, warning_messages, ticks_per_measure, rhythmic_hierarchy_max_depth, rhythmic_hierarchy_of_beat);
		}
	}
	check_dissonance_handling(sonority_array, error_messages, warning_messages, ticks_per_measure, tonic, dominant, leading_tone, rhythmic_hierarchy_array); // Only check lowest level

#ifdef SHOW_COUNTERPOINT_CHECKS
	print_messages(error_messages, warning_messages);
#endif // SHOW_COUNTERPOINT_CHECKS

	return { error_messages, warning_messages };
}