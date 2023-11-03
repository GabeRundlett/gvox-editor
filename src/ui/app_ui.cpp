#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>

namespace {
    Rml::Context *context{};
    bool run_loop = true;

    bool show_text = true;
    Rml::String animal = "dog";
} // namespace

#include "app_ui.hpp"

#include <daxa/command_recorder.hpp>

void LoadFonts() {
    const Rml::String directory = "C:/dev/downloads/RmlUi/Samples/assets/";

    struct FontFace {
        const char *filename;
        bool fallback_face;
    };
    auto font_faces = std::array{
        FontFace{"LatoLatin-Regular.ttf", false},
        FontFace{"LatoLatin-Italic.ttf", false},
        FontFace{"LatoLatin-Bold.ttf", false},
        FontFace{"LatoLatin-BoldItalic.ttf", false},
        FontFace{"NotoEmoji-Regular.ttf", true},
    };

    for (const FontFace &face : font_faces) {
        Rml::LoadFontFace(directory + face.filename, face.fallback_face);
    }
}

AppUi::AppUi(daxa::Device device)
    : app_windows([&]() {
        auto result = std::vector<AppWindow>{};
        result.emplace_back(device, daxa_i32vec2{800, 600});
        return result; }()),
      render_interface(device, app_windows[0].swapchain.get_format()) {

    auto &app_window = app_windows[0];
    app_window.on_close = [&]() { should_close.store(true); };

    system_interface.SetWindow(app_window.glfw_window.get());
    // Install the custom interfaces constructed by the backend before initializing RmlUi.
    Rml::SetSystemInterface(&system_interface);
    Rml::SetRenderInterface(&render_interface);

    // RmlUi initialisation.
    Rml::Initialise();

    context = Rml::CreateContext("main", Rml::Vector2i(app_window.size.x, app_window.size.y));
    app_window.rml_context = context;

    Rml::Debugger::Initialise(context);

    // Set up data bindings to synchronize application data.
    if (Rml::DataModelConstructor constructor = context->CreateDataModel("animals")) {
        constructor.Bind("show_text", &show_text);
        constructor.Bind("animal", &animal);
    }

    LoadFonts();

    Rml::ElementDocument *document = context->LoadDocument("src/ui/hello_world.rml");
    document->Show();

    // Replace and style some text in the loaded document.
    Rml::Element *element = document->GetElementById("world");
    element->SetInnerRML(reinterpret_cast<const char *>(u8"🌍"));
    element->SetProperty("font-size", "1.5em");
}

AppUi::~AppUi() {
    Rml::Shutdown();
}

auto ProcessKeyDownShortcuts(Rml::Context *context, Rml::Input::KeyIdentifier key, int key_modifier, float native_dp_ratio, bool priority) -> bool {
    if (context == nullptr) {
        return true;
    }

    // Result should return true to allow the event to propagate to the next handler.
    bool result = false;

    // This function is intended to be called twice by the backend, before and after submitting the key event to the context. This way we can
    // intercept shortcuts that should take priority over the context, and then handle any shortcuts of lower priority if the context did not
    // intercept it.
    if (priority) {
        // Priority shortcuts are handled before submitting the key to the context.

        // Toggle debugger and set dp-ratio using Ctrl +/-/0 keys.
        if (key == Rml::Input::KI_F8) {
            Rml::Debugger::SetVisible(!Rml::Debugger::IsVisible());
        } else if (key == Rml::Input::KI_0 && ((key_modifier & Rml::Input::KM_CTRL) != 0)) {
            context->SetDensityIndependentPixelRatio(native_dp_ratio);
        } else if (key == Rml::Input::KI_1 && ((key_modifier & Rml::Input::KM_CTRL) != 0)) {
            context->SetDensityIndependentPixelRatio(1.f);
        } else if ((key == Rml::Input::KI_OEM_MINUS || key == Rml::Input::KI_SUBTRACT) && ((key_modifier & Rml::Input::KM_CTRL) != 0)) {
            const float new_dp_ratio = Rml::Math::Max(context->GetDensityIndependentPixelRatio() / 1.2f, 0.5f);
            context->SetDensityIndependentPixelRatio(new_dp_ratio);
        } else if ((key == Rml::Input::KI_OEM_PLUS || key == Rml::Input::KI_ADD) && ((key_modifier & Rml::Input::KM_CTRL) != 0)) {
            const float new_dp_ratio = Rml::Math::Min(context->GetDensityIndependentPixelRatio() * 1.2f, 2.5f);
            context->SetDensityIndependentPixelRatio(new_dp_ratio);
        } else {
            // Propagate the key down event to the context.
            result = true;
        }
    } else {
        // We arrive here when no priority keys are detected and the key was not consumed by the context. Check for shortcuts of lower priority.
        if (key == Rml::Input::KI_R && ((key_modifier & Rml::Input::KM_CTRL) != 0)) {
            for (int i = 0; i < context->GetNumDocuments(); i++) {
                Rml::ElementDocument *document = context->GetDocument(i);
                const Rml::String &src = document->GetSourceURL();
                if (src.size() > 4 && src.substr(src.size() - 4) == ".rml") {
                    document->ReloadStyleSheet();
                }
            }
        } else {
            result = true;
        }
    }

    return result;
}

void AppUi::update() {
    for (auto &app_window : app_windows) {
        app_window.key_down_callback = ProcessKeyDownShortcuts;
        app_window.update();
    }

    if (run_loop == false) {
        should_close.store(true);
    }
}

void AppUi::render(daxa::CommandRecorder &recorder, daxa::ImageId target_image) {
    context->Update();
    render_interface.begin_frame(target_image, recorder);
    context->Render();
    render_interface.end_frame(target_image);
}
