#include "ImGuiManager.hh"

#include "ImGuiBitmapViewer.hh"
#include "ImGuiBreakPoints.hh"
#include "ImGuiCharacter.hh"
#include "ImGuiCheatFinder.hh"
#include "ImGuiCpp.hh"
#include "ImGuiConnector.hh"
#include "ImGuiConsole.hh"
#include "ImGuiDebugger.hh"
#include "ImGuiDiskManipulator.hh"
#include "ImGuiHelp.hh"
#include "ImGuiKeyboard.hh"
#include "ImGuiMachine.hh"
#include "ImGuiMedia.hh"
#include "ImGuiMessages.hh"
#include "ImGuiOpenFile.hh"
#include "ImGuiPalette.hh"
#include "ImGuiOsdIcons.hh"
#include "ImGuiPart.hh"
#include "ImGuiReverseBar.hh"
#include "ImGuiSettings.hh"
#include "ImGuiSoundChip.hh"
#include "ImGuiSpriteViewer.hh"
#include "ImGuiSymbols.hh"
#include "ImGuiTools.hh"
#include "ImGuiTrainer.hh"
#include "ImGuiUtils.hh"
#include "ImGuiVdpRegs.hh"
#include "ImGuiWatchExpr.hh"


#include "CartridgeSlotManager.hh"
#include "CommandException.hh"
#include "Display.hh"
#include "Event.hh"
#include "EventDistributor.hh"
#include "FileContext.hh"
#include "FileOperations.hh"
#include "FilePool.hh"
#include "Reactor.hh"
#include "RealDrive.hh"
#include "RomDatabase.hh"
#include "RomInfo.hh"

#include "stl.hh"
#include "strCat.hh"

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>
#include <imgui_internal.h>
#include <CustomFont.ii> // icons for ImGuiFileDialog

#include <SDL.h>

namespace openmsx {

using namespace std::literals;

ImFont* ImGuiManager::addFont(zstring_view filename, int fontSize)
{
	auto& io = ImGui::GetIO();
	if (!filename.empty()) {
		try {
			const auto& context = systemFileContext();

			File file(context.resolve(FileOperations::join("skins", filename)));
			auto fileSize = file.getSize();
			auto ttfData = std::span(
				static_cast<uint8_t*>(ImGui::MemAlloc(fileSize)), fileSize);
			file.read(ttfData);

			static const std::array<ImWchar, 2*6 + 1> ranges = {
				0x0020, 0x00FF, // Basic Latin + Latin Supplement
				0x0370, 0x03FF, // Greek and Coptic
				0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
				//0x0E00, 0x0E7F, // Thai
				//0x2000, 0x206F, // General Punctuation
				//0x2DE0, 0x2DFF, // Cyrillic Extended-A
				0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
				0x3131, 0x3163, // Korean alphabets
				0x31F0, 0x31FF, // Katakana Phonetic Extensions
				//0x4e00, 0x9FAF, // CJK Ideograms
				//0xA640, 0xA69F, // Cyrillic Extended-B
				//0xAC00, 0xD7A3, // Korean characters
				//0xFF00, 0xFFEF, // Half-width characters
				0
			};
			return io.Fonts->AddFontFromMemoryTTF(
				ttfData.data(), // transfer ownership of 'ttfData' buffer
				narrow<int>(ttfData.size()), narrow<float>(fontSize),
				nullptr, ranges.data());
		} catch (MSXException& e) {
			getCliComm().printWarning("Couldn't load font: ", filename, ": ", e.getMessage(),
						". Reverted to builtin font");
		}
	}
	return io.Fonts->AddFontDefault(); // embedded "ProggyClean.ttf", size 13
}

void ImGuiManager::loadFont()
{
	ImGuiIO& io = ImGui::GetIO();

	assert(fontProp == nullptr);
	fontProp = addFont(fontPropFilename.getString(), fontPropSize.getInt());

	//// load icon font file (CustomFont.cpp), only in the default font
	static const ImWchar icons_ranges[] = { ICON_MIN_IGFD, ICON_MAX_IGFD, 0 };
	ImFontConfig icons_config; icons_config.MergeMode = true; icons_config.PixelSnapH = true;
	io.Fonts->AddFontFromMemoryCompressedBase85TTF(FONT_ICON_BUFFER_NAME_IGFD, 15.0f, &icons_config, icons_ranges);
	// load debugger icons, also only in default font
	debugger->loadIcons();

	assert(fontMono == nullptr);
	fontMono = addFont(fontMonoFilename.getString(), fontMonoSize.getInt());
}

void ImGuiManager::reloadFont()
{
	fontProp = fontMono = nullptr;

	ImGui_ImplOpenGL3_DestroyFontsTexture();

	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->Clear();
	loadFont();
	io.Fonts->Build();

	ImGui_ImplOpenGL3_CreateFontsTexture();
}

void ImGuiManager::initializeImGui()
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard |
	                  //ImGuiConfigFlags_NavEnableGamepad | // TODO revisit this later
	                  ImGuiConfigFlags_DockingEnable |
	                  ImGuiConfigFlags_ViewportsEnable;
	static auto iniFilename = systemFileContext().resolveCreate("imgui.ini");
	io.IniFilename = iniFilename.c_str();

	loadFont();
}

static void cleanupImGui()
{
	ImGui::DestroyContext();
}


ImGuiManager::ImGuiManager(Reactor& reactor_)
	: ImGuiPart(*this)
	, reactor(reactor_)
	, fontPropFilename(reactor.getCommandController(), "gui_font_default_filename", "TTF font filename for the default GUI font", "Vera.ttf.gz")
	, fontMonoFilename(reactor.getCommandController(), "gui_font_mono_filename", "TTF font filename for the monospaced GUI font", "VeraMono.ttf.gz")
	, fontPropSize(reactor.getCommandController(), "gui_font_default_size", "size for the default GUI font", 13, 9, 72)
	, fontMonoSize(reactor.getCommandController(), "gui_font_mono_size", "size for the monospaced GUI font", 13, 9, 72)
	, machine(std::make_unique<ImGuiMachine>(*this))
	, debugger(std::make_unique<ImGuiDebugger>(*this))
	, breakPoints(std::make_unique<ImGuiBreakPoints>(*this))
	, symbols(std::make_unique<ImGuiSymbols>(*this))
	, watchExpr(std::make_unique<ImGuiWatchExpr>(*this))
	, bitmap(std::make_unique<ImGuiBitmapViewer>(*this))
	, character(std::make_unique<ImGuiCharacter>(*this))
	, sprite(std::make_unique<ImGuiSpriteViewer>(*this))
	, vdpRegs(std::make_unique<ImGuiVdpRegs>(*this))
	, palette(std::make_unique<ImGuiPalette>())
	, reverseBar(std::make_unique<ImGuiReverseBar>(*this))
	, help(std::make_unique<ImGuiHelp>())
	, osdIcons(std::make_unique<ImGuiOsdIcons>(*this))
	, openFile(std::make_unique<ImGuiOpenFile>(*this))
	, media(std::make_unique<ImGuiMedia>(*this))
	, connector(std::make_unique<ImGuiConnector>(*this))
	, tools(std::make_unique<ImGuiTools>(*this))
	, trainer(std::make_unique<ImGuiTrainer>(*this))
	, cheatFinder(std::make_unique<ImGuiCheatFinder>(*this))
	, diskManipulator(std::make_unique<ImGuiDiskManipulator>(*this))
	, settings(std::make_unique<ImGuiSettings>(*this))
	, soundChip(std::make_unique<ImGuiSoundChip>(*this))
	, keyboard(std::make_unique<ImGuiKeyboard>(*this))
	, console(std::make_unique<ImGuiConsole>(*this))
	, messages(std::make_unique<ImGuiMessages>(*this))
	, windowPos{SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED}
{
	initializeImGui();

	ImGuiSettingsHandler ini_handler;
	ini_handler.TypeName = "openmsx";
	ini_handler.TypeHash = ImHashStr("openmsx");
	ini_handler.UserData = this;
	//ini_handler.ClearAllFn = [](ImGuiContext*, ImGuiSettingsHandler* handler) { // optional
	//      // Clear all settings data
	//      static_cast<ImGuiManager*>(handler->UserData)->iniClearAll();
	//};
	ini_handler.ReadInitFn = [](ImGuiContext*, ImGuiSettingsHandler* handler) { // optional
	      // Read: Called before reading (in registration order)
	      static_cast<ImGuiManager*>(handler->UserData)->iniReadInit();
	};
	ini_handler.ReadOpenFn = [](ImGuiContext*, ImGuiSettingsHandler* handler, const char* name) -> void* { // required
	        // Read: Called when entering into a new ini entry e.g. "[Window][Name]"
	        return static_cast<ImGuiManager*>(handler->UserData)->iniReadOpen(name);
	};
	ini_handler.ReadLineFn = [](ImGuiContext*, ImGuiSettingsHandler* handler, void* entry, const char* line) { // required
	        // Read: Called for every line of text within an ini entry
	        static_cast<ImGuiManager*>(handler->UserData)->loadLine(entry, line);
	};
	ini_handler.ApplyAllFn = [](ImGuiContext*, ImGuiSettingsHandler* handler) { // optional
	      // Read: Called after reading (in registration order)
	      static_cast<ImGuiManager*>(handler->UserData)->iniApplyAll();
	};
	ini_handler.WriteAllFn = [](ImGuiContext*, ImGuiSettingsHandler* handler, ImGuiTextBuffer* out_buf) { // required
	        // Write: Output every entries into 'out_buf'
	        static_cast<ImGuiManager*>(handler->UserData)->iniWriteAll(*out_buf);
	};
	ImGui::AddSettingsHandler(&ini_handler);

	auto& eventDistributor = reactor.getEventDistributor();
	eventDistributor.registerEventListener(EventType::MOUSE_BUTTON_UP,   *this, EventDistributor::IMGUI);
	eventDistributor.registerEventListener(EventType::MOUSE_BUTTON_DOWN, *this, EventDistributor::IMGUI);
	eventDistributor.registerEventListener(EventType::MOUSE_MOTION,      *this, EventDistributor::IMGUI);
	eventDistributor.registerEventListener(EventType::MOUSE_WHEEL,       *this, EventDistributor::IMGUI);
	eventDistributor.registerEventListener(EventType::KEY_UP,            *this, EventDistributor::IMGUI);
	eventDistributor.registerEventListener(EventType::KEY_DOWN,          *this, EventDistributor::IMGUI);
	eventDistributor.registerEventListener(EventType::TEXT,              *this, EventDistributor::IMGUI);
	eventDistributor.registerEventListener(EventType::WINDOW,            *this, EventDistributor::IMGUI);
	eventDistributor.registerEventListener(EventType::FILE_DROP, *this);
	eventDistributor.registerEventListener(EventType::IMGUI_DELAYED_ACTION, *this);
	eventDistributor.registerEventListener(EventType::BREAK, *this);

	fontPropFilename.attach(*this);
	fontMonoFilename.attach(*this);
	fontPropSize.attach(*this);
	fontMonoSize.attach(*this);

	// In order that they appear in the menubar
	append(parts, std::initializer_list<ImGuiPart*>{
		this,
		machine.get(), media.get(), connector.get(), reverseBar.get(), tools.get(), settings.get(),
		debugger.get(), help.get(), soundChip.get(), keyboard.get(), symbols.get(), breakPoints.get(),
		watchExpr.get(), bitmap.get(), character.get(), sprite.get(), vdpRegs.get(), palette.get(),
		osdIcons.get(), openFile.get(), console.get(), messages.get(), trainer.get(), cheatFinder.get(),
		diskManipulator.get()});
}

ImGuiManager::~ImGuiManager()
{
	fontMonoSize.detach(*this);
	fontPropSize.detach(*this);
	fontMonoFilename.detach(*this);
	fontPropFilename.detach(*this);

	auto& eventDistributor = reactor.getEventDistributor();
	eventDistributor.unregisterEventListener(EventType::BREAK, *this);
	eventDistributor.unregisterEventListener(EventType::IMGUI_DELAYED_ACTION, *this);
	eventDistributor.unregisterEventListener(EventType::FILE_DROP, *this);
	eventDistributor.unregisterEventListener(EventType::WINDOW, *this);
	eventDistributor.unregisterEventListener(EventType::TEXT, *this);
	eventDistributor.unregisterEventListener(EventType::KEY_DOWN, *this);
	eventDistributor.unregisterEventListener(EventType::KEY_UP, *this);
	eventDistributor.unregisterEventListener(EventType::MOUSE_WHEEL, *this);
	eventDistributor.unregisterEventListener(EventType::MOUSE_MOTION, *this);
	eventDistributor.unregisterEventListener(EventType::MOUSE_BUTTON_DOWN, *this);
	eventDistributor.unregisterEventListener(EventType::MOUSE_BUTTON_UP, *this);

	cleanupImGui();
}

void ImGuiManager::save(ImGuiTextBuffer& buf)
{
	// We cannot query "reactor.getDisplay().getWindowPosition()" here
	// because display may already be destroyed. Instead Display pushes
	// window position to here
	savePersistent(buf, *this, persistentElements);
}

void ImGuiManager::loadLine(std::string_view name, zstring_view value)
{
	loadOnePersistent(name, value, *this, persistentElements);
}

void ImGuiManager::loadEnd()
{
	reactor.getDisplay().setWindowPosition(windowPos);
}

Interpreter& ImGuiManager::getInterpreter()
{
	return reactor.getInterpreter();
}

CliComm& ImGuiManager::getCliComm()
{
	return reactor.getCliComm();
}

std::optional<TclObject> ImGuiManager::execute(TclObject command)
{
	try {
		return command.executeCommand(getInterpreter());
	} catch (CommandException&) {
		// ignore
		return {};
	}
}

void ImGuiManager::executeDelayed(std::function<void()> action)
{
	delayedActionQueue.push_back(std::move(action));
	reactor.getEventDistributor().distributeEvent(ImGuiDelayedActionEvent());
}

void ImGuiManager::executeDelayed(TclObject command,
                                  std::function<void(const TclObject&)> ok,
                                  std::function<void(const std::string&)> error)
{
	executeDelayed([this, command, ok, error]() mutable {
		try {
			auto result = command.executeCommand(getInterpreter());
			if (ok) ok(result);
		} catch (CommandException& e) {
			if (error) error(e.getMessage());
		}
	});
}

void ImGuiManager::executeDelayed(TclObject command,
                                  std::function<void(const TclObject&)> ok)
{
	executeDelayed(std::move(command), ok,
		[this](const std::string& message) { this->printError(message); });
}

void ImGuiManager::printError(std::string_view message)
{
	getCliComm().printError(message);
}

int ImGuiManager::signalEvent(const Event& event)
{
	if (auto* evt = get_event_if<SdlEvent>(event)) {
		const SDL_Event& sdlEvent = evt->getSdlEvent();
		ImGui_ImplSDL2_ProcessEvent(&sdlEvent);
		ImGuiIO& io = ImGui::GetIO();
		if ((io.WantCaptureMouse &&
		     sdlEvent.type == one_of(SDL_MOUSEMOTION, SDL_MOUSEWHEEL,
		                             SDL_MOUSEBUTTONDOWN, SDL_MOUSEBUTTONUP)) ||
		    (io.WantCaptureKeyboard &&
		     sdlEvent.type == one_of(SDL_KEYDOWN, SDL_KEYUP, SDL_TEXTINPUT))) {
			return EventDistributor::MSX; // block event for the MSX
		}
	} else {
		switch (getType(event)) {
		case EventType::IMGUI_DELAYED_ACTION: {
			for (auto& action : delayedActionQueue) {
				std::invoke(action);
			}
			delayedActionQueue.clear();
			break;
		}
		case EventType::FILE_DROP: {
			const auto& fde = get_event<FileDropEvent>(event);
			droppedFile = fde.getFileName();
			handleDropped = true;
			break;
		}
		case EventType::BREAK:
			debugger->signalBreak();
			break;
		default:
			UNREACHABLE;
		}
	}
	return 0;
}

void ImGuiManager::update(const Setting& /*setting*/) noexcept
{
	needReloadFont = true;
}

// TODO share code with ImGuiMedia
static std::vector<std::string> getDrives(MSXMotherBoard* motherBoard)
{
	std::vector<std::string> result;
	if (!motherBoard) return result;

	std::string driveName = "diskX";
	auto drivesInUse = RealDrive::getDrivesInUse(*motherBoard);
	for (auto i : xrange(RealDrive::MAX_DRIVES)) {
		if (!(*drivesInUse)[i]) continue;
		driveName[4] = char('a' + i);
		result.push_back(driveName);
	}
	return result;
}

static std::vector<std::string> getSlots(MSXMotherBoard* motherBoard)
{
	std::vector<std::string> result;
	if (!motherBoard) return result;

	auto& slotManager = motherBoard->getSlotManager();
	std::string cartName = "cartX";
	for (auto slot : xrange(CartridgeSlotManager::MAX_SLOTS)) {
		if (!slotManager.slotExists(slot)) continue;
		cartName[4] = char('a' + slot);
		result.push_back(cartName);
	}
	return result;
}

void ImGuiManager::preNewFrame()
{
	if (!loadIniFile.empty()) {
		ImGui::LoadIniSettingsFromDisk(loadIniFile.c_str());
		loadIniFile.clear();
	}
	if (needReloadFont) {
		needReloadFont = false;
		reloadFont();
	}
}

void ImGuiManager::paintImGui()
{
	auto* motherBoard = reactor.getMotherBoard();
	for (auto* part : parts) {
		part->paint(motherBoard);
	}
	if (openFile->mustPaint(ImGuiOpenFile::Painter::MANAGER)) {
		openFile->doPaint();
	}

	auto drawMenu = [&]{
		for (auto* part : parts) {
			part->showMenu(motherBoard);
		}
	};
	if (mainMenuBarUndocked) {
		im::Window("openMSX main menu", &mainMenuBarUndocked, ImGuiWindowFlags_MenuBar, [&]{
			im::MenuBar([&]{
				if (ImGui::ArrowButton("re-dock-button", ImGuiDir_Down)) {
					mainMenuBarUndocked = false;
				}
				simpleToolTip("Dock the menu bar in the main openMSX window.");
				drawMenu();
			});
		});
	} else {
		bool active = ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) ||
		              ImGui::IsWindowFocused(ImGuiHoveredFlags_AnyWindow);
		if (active != guiActive) {
			guiActive = active;
			auto& eventDistributor = reactor.getEventDistributor();
			eventDistributor.distributeEvent(ImGuiActiveEvent(active));
		}
		menuAlpha = [&] {
			if (!menuFade) return 1.0f;
			auto target = active ? 1.0f : 0.0001f;
			auto period = active ? 0.5f : 5.0f;
			return calculateFade(menuAlpha, target, period);
		}();
		im::StyleVar(ImGuiStyleVar_Alpha, menuAlpha, [&]{
			im::MainMenuBar([&]{
				if (ImGui::ArrowButton("undock-button", ImGuiDir_Up)) {
					mainMenuBarUndocked = true;
				}
				simpleToolTip("Undock the menu bar from the main openMSX window.");
				drawMenu();
			});
		});
	}

	// drag and drop  (move this to ImGuiMedia ?)
	auto insert2 = [&](std::string_view displayName, TclObject cmd) {
		auto message = strCat("Inserted ", droppedFile, " in ", displayName);
		executeDelayed(cmd, [this, message](const TclObject&){
			insertedInfo = message;
			openInsertedInfo = true;
		});
	};
	auto insert = [&](std::string_view displayName, std::string_view cmd) {
		insert2(displayName, makeTclList(cmd, "insert", droppedFile));
	};
	if (handleDropped) {
		handleDropped = false;
		insertedInfo.clear();

		auto category = execute(makeTclList("openmsx_info", "file_type_category", droppedFile))->getString();
		if (category == "unknown" && FileOperations::isDirectory(droppedFile)) {
			category = "disk";
		}

		auto error = [&](auto&& ...message) {
			executeDelayed(makeTclList("error", strCat(message...)));
		};
		auto cantHandle = [&](auto&& ...message) {
			error("Can't handle dropped file ", droppedFile, ": ", message...);
		};
		auto notPresent = [&](const auto& mediaType) {
			cantHandle("no ", mediaType, " present.");
		};

		auto testMedia = [&](std::string_view displayName, std::string_view cmd) {
			if (auto cmdResult = execute(TclObject(cmd))) {
				insert(displayName, cmd);
			} else {
				notPresent(displayName);
			}
		};

		if (category == "disk") {
			auto list = getDrives(motherBoard);
			if (list.empty()) {
				notPresent("disk drive");
			} else if (list.size() == 1) {
				const auto& drive = list.front();
				insert(strCat("disk drive ", char(drive.back() - 'a' + 'A')), drive);
			} else {
				selectList = std::move(list);
				ImGui::OpenPopup("select-drive");
			}
		} else if (category == "rom") {
			auto list = getSlots(motherBoard);
			if (list.empty()) {
				notPresent("cartridge slot");
				return;
			}
			selectedMedia = list.front();
			selectList = std::move(list);
			if (auto sha1 = reactor.getFilePool().getSha1Sum(droppedFile)) {
				romInfo = reactor.getSoftwareDatabase().fetchRomInfo(*sha1);
			} else {
				romInfo = nullptr;
			}
			selectedRomType = romInfo ? romInfo->getRomType()
			                          : ROM_UNKNOWN; // auto-detect
			ImGui::OpenPopup("select-cart");
		} else if (category == "cassette") {
			testMedia("casette port", "cassetteplayer");
		} else if (category == "laserdisc") {
			testMedia("laser disc player", "laserdiscplayer");
		} else if (category == "savestate") {
			executeDelayed(makeTclList("loadstate", droppedFile));
		} else if (category == "replay") {
			executeDelayed(makeTclList("reverse", "loadreplay", droppedFile));
		} else if (category == "script") {
			executeDelayed(makeTclList("source", droppedFile));
		} else if (FileOperations::getExtension(droppedFile) == ".txt") {
			executeDelayed(makeTclList("type_from_file", droppedFile));
		} else {
			cantHandle("unknown file type");
		}
	}
	im::Popup("select-drive", [&]{
		ImGui::TextUnformatted(tmpStrCat("Select disk drive for ", droppedFile));
		auto n = std::min(3.5f, narrow<float>(selectList.size()));
		auto height = n * ImGui::GetTextLineHeightWithSpacing() + ImGui::GetStyle().FramePadding.y;
		im::ListBox("##select-media", {-FLT_MIN, height}, [&]{
			for (const auto& item : selectList) {
				auto drive = item.back() - 'a';
				auto display = strCat(char('A' + drive), ": ", media->displayNameForDriveContent(drive, true));
				if (ImGui::Selectable(display.c_str())) {
					insert(strCat("disk drive ", char(drive + 'A')), item);
					ImGui::CloseCurrentPopup();
				}
			}
		});
	});
	im::Popup("select-cart", [&]{
		ImGui::TextUnformatted(strCat("Filename: ", droppedFile));
		ImGui::Separator();

		if (!romInfo) {
			ImGui::TextUnformatted("ROM not present in software database"sv);
		}
		im::Table("##extension-info", 2, [&]{
			const char* buf = reactor.getSoftwareDatabase().getBufferStart();
			ImGui::TableSetupColumn("description", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);

			if (romInfo) {
				ImGuiMedia::printDatabase(*romInfo, buf);
			}
			if (ImGui::TableNextColumn()) {
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted("Mapper"sv);
			}
			if (ImGui::TableNextColumn()) {
				ImGuiMedia::selectMapperType("##mapper-type", selectedRomType);
			}
		});
		ImGui::Separator();

		if (selectList.size() > 1) {
			const auto& slotManager = motherBoard->getSlotManager();
			ImGui::TextUnformatted("Select cartridge slot"sv);
			auto n = std::min(3.5f, narrow<float>(selectList.size()));
			auto height = n * ImGui::GetTextLineHeightWithSpacing() + ImGui::GetStyle().FramePadding.y;
			im::ListBox("##select-media", {-FLT_MIN, height}, [&]{
				for (const auto& item : selectList) {
					auto slot = item.back() - 'a';
					auto display = strCat(
						char('A' + slot),
						" (", slotManager.getPsSsString(slot), "): ",
						media->displayNameForSlotContent(slotManager, slot, true));

					if (ImGui::Selectable(display.c_str(), item == selectedMedia)) {
						selectedMedia = item;
					}
				}
			});
		}

		ImGui::Checkbox("Reset MSX on inserting ROM", &media->resetOnInsertRom);

		if (ImGui::Button("Insert ROM")) {
			auto cmd = makeTclList(selectedMedia, "insert", droppedFile);
			if (selectedRomType != ROM_UNKNOWN) {
				cmd.addListElement("-romtype", RomInfo::romTypeToName(selectedRomType));
			}
			insert2(strCat("cartridge slot ", char(selectedMedia.back() - 'a' + 'A')), cmd);
			if (media->resetOnInsertRom) {
				executeDelayed(TclObject("reset"));
			}
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			ImGui::CloseCurrentPopup();
		}
	});
	if (openInsertedInfo) {
		openInsertedInfo = false;
		insertedInfoTimeout = 3.0f;
		ImGui::OpenPopup("inserted-info");
	}
	im::Popup("inserted-info", [&]{
		insertedInfoTimeout -= ImGui::GetIO().DeltaTime;
		if (insertedInfoTimeout <= 0.0f || insertedInfo.empty()) {
			ImGui::CloseCurrentPopup();
		}
		im::TextWrapPos(ImGui::GetFontSize() * 35.0f, [&]{
			ImGui::TextUnformatted(insertedInfo);
		});
	});
}

void ImGuiManager::iniReadInit()
{
	for (auto* part : parts) {
		part->loadStart();
	}
}

void* ImGuiManager::iniReadOpen(std::string_view name)
{
	for (auto* part : parts) {
		if (part->iniName() == name) return part;
	}
	return nullptr;
}

void ImGuiManager::loadLine(void* entry, const char* line_)
{
	zstring_view line = line_;
	auto pos = line.find('=');
	if (pos == zstring_view::npos) return;
	std::string_view name = line.substr(0, pos);
	zstring_view value = line.substr(pos + 1);

	assert(entry);
	static_cast<ImGuiPart*>(entry)->loadLine(name, value);
}

void ImGuiManager::iniApplyAll()
{
	for (auto* part : parts) {
		part->loadEnd();
	}
}

void ImGuiManager::iniWriteAll(ImGuiTextBuffer& buf)
{
	for (auto* part : parts) {
		if (auto name = part->iniName(); !name.empty()) {
			buf.appendf("[openmsx][%s]\n", name.c_str());
			part->save(buf);
			buf.append("\n");
		}
	}
}

} // namespace openmsx
