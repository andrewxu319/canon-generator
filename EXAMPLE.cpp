#include "mx/api/DocumentManager.h"
#include "mx/api/ScoreData.h"

#include <string>
#include <iostream>
#include <cstdint>
#include <sstream>

#define MX_IS_A_SUCCESS 0
#define MX_IS_A_FAILURE 1

constexpr const char* const xml = R"(
<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<!DOCTYPE score-partwise PUBLIC "-//Recordare//DTD MusicXML 3.0 Partwise//EN" "http://www.musicxml.org/dtds/partwise.dtd">
<score-partwise>
  <work>
    <work-title>Mx Example</work-title>
  </work>
  <identification>
    <creator type="composer">Matthew James Briggs</creator>
    <rights type="copyright">Copyright (c) 2019</rights>
  </identification>
  <part-list>
    <score-part id="ID">
      <part-name print-object="no">Flute</part-name>
      <part-name-display>
        <display-text>Flute</display-text>
      </part-name-display>
      <part-abbreviation print-object="no">Fl.</part-abbreviation>
      <part-abbreviation-display>
        <display-text>Fl.</display-text>
      </part-abbreviation-display>
      <score-instrument id="ID1000000">
        <instrument-name />
        <instrument-sound>wind.flutes.flute</instrument-sound>
      </score-instrument>
      <midi-instrument id="ID">
        <midi-channel>1</midi-channel>
        <midi-program>74</midi-program>
      </midi-instrument>
    </score-part>
  </part-list>
  <part id="ID">
    <measure number="1">
      <attributes>
        <divisions>4</divisions>
        <time>
          <beats>4</beats>
          <beat-type>4</beat-type>
        </time>
        <clef>
          <sign>G</sign>
          <line>2</line>
        </clef>
      </attributes>
      <note>
        <pitch>
          <step>D</step>
          <alter>1</alter>
          <octave>5</octave>
        </pitch>
        <duration>8</duration>
        <voice>1</voice>
        <type>half</type>
        <accidental>sharp</accidental>
      </note>
      <note>
        <pitch>
          <step>E</step>
          <octave>5</octave>
        </pitch>
        <duration>2</duration>
        <voice>1</voice>
        <type>eighth</type>
        <beam number="1">begin</beam>
      </note>
      <note>
        <pitch>
          <step>F</step>
          <octave>5</octave>
        </pitch>
        <duration>2</duration>
        <voice>1</voice>
        <type>eighth</type>
        <beam number="1">end</beam>
      </note>
      <note>
        <pitch>
          <step>E</step>
          <octave>5</octave>
        </pitch>
        <duration>4</duration>
        <voice>1</voice>
        <type>quarter</type>
      </note>
    </measure>
  </part>
</score-partwise>
)";

int main1(int argc, const char* argv[]) // main1
{
    using namespace mx::api;

    // create a reference to the singleton which holds documents in memory for us
    auto& mgr = DocumentManager::getInstance();

    // place the xml from above into a stream object
    std::istringstream istr{ xml };

    // ask the document manager to parse the xml into memory for us, returns a document ID.
    const auto documentID = mgr.createFromStream(istr);

    // get the structural representation of the score from the document manager
    const auto score = mgr.getData(documentID);

    // we need to explicitly destroy the document from memory
    mgr.destroyDocument(documentID);

    // make sure we have exactly one part
    if (score.parts.size() != 1)
    {
        return MX_IS_A_FAILURE;
    }

    // drill down into the data structure to retrieve the note
    const auto& part = score.parts.at(0);
    const auto& measure = part.measures.at(0);
    const auto& staff = measure.staves.at(0);
    const auto& voice = staff.voices.at(0);
    const auto& note = voice.notes.at(0);

    if (note.durationData.durationName != DurationName::whole)
    {
        return MX_IS_A_FAILURE;
    }

    if (note.pitchData.step != Step::c)
    {
        return MX_IS_A_FAILURE;
    }

    std::cout << note.pitchData.alter;

    return MX_IS_A_SUCCESS;
}