
#include "project.h"

int main()
{
    using namespace al;
    using namespace al::engine;

    Root root({
        {
            // default window properties
        },
        {
            // default sound properties
        },
        gigabytes(1u)
    });

    ErrorInfo error = root.get_construction_result();
    al_assert(error);

    // SoundId id = root.get_sound_system()->load_sound(SoundSystem::SourceType::WAV, "Assets\\sound.wav");
    // root.get_sound_system()->dbg_play_single_wav(id);

    ApplicationWindowInput input;

    auto printTime = [](double time) { printf("%f\n", time); };
    auto refPrintTime = std::ref(printTime);
    while (true)
    {
        //ScopeTimer timer{ refPrintTime };

        root.get_input(&input);

        if (input.generalInput.get_flag(ApplicationWindowInput::GeneralInputFlags::CLOSE_BUTTON_PRESSED))
            break;
        if (input.keyboard.buttons.get_flag(ApplicationWindowInput::KeyboardInputFlags::ESCAPE))
            break;

        root.get_renderer()->commit();
    }

    return 0;
}
