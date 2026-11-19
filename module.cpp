// raylib's KEY_* and Matrix clash with y-- names
#define KEY_ENTER RAY_ENTER
#define KEY_HOME RAY_HOME
#define KEY_RIGHT RAY_RIGHT
#define KEY_LEFT RAY_LEFT
#define KEY_DOWN RAY_DOWN
#define KEY_UP RAY_UP
#define KEY_BACKSPACE RAY_BACKSPACE
#define KEY_END RAY_END
#define Matrix RaylibMatrix
#include "raylib.h"
#undef KEY_ENTER
#undef KEY_HOME
#undef KEY_RIGHT
#undef KEY_LEFT
#undef KEY_UP
#undef KEY_DOWN
#undef KEY_BACKSPACE
#undef KEY_END
#undef Matrix
#include "ylang/vm.hpp"
#include <iostream>

static inline Color ValueToColor(const Value &v, int l, int c) {
	if (v.type != ValueType::INSTANCE)
		throw TypeError("Expected Color object", l, c);
	auto *inst = static_cast<InstanceObject *>(v.ref.get());
	if (inst->klass->name != "Color")
		throw TypeError("Expected Color object", l, c);
	return Color{(unsigned char)inst->fields["r"].asInt(),
		(unsigned char)inst->fields["g"].asInt(),
		(unsigned char)inst->fields["b"].asInt(),
		(unsigned char)inst->fields["a"].asInt()};
}
static inline Rectangle ValueToRect(const Value &v, int l, int c) {
	if (v.type != ValueType::INSTANCE)
		throw TypeError("Expected Rectangle object", l, c);
	auto *inst = static_cast<InstanceObject *>(v.ref.get());
	if (inst->klass->name != "Rectangle")
		throw TypeError("Expected Rectangle object", l, c);
	return Rectangle{(float)inst->fields["x"].asInt(),
		(float)inst->fields["y"].asInt(),
		(float)inst->fields["width"].asInt(),
		(float)inst->fields["height"].asInt()};
}
static inline ::Vector2 ValueToVector2(const Value &v, int l, int c) {
	if (v.type != ValueType::VECTOR)
		throw TypeError("Expected Vector object", l, c);
	auto *vec = static_cast<VectorObject *>(v.ref.get());
	if (vec->elements.size() < 2)
		throw ValueError("Vector must have at least 2 elements for Vector2", l,
			c);
	return ::Vector2{(float)vec->elements[0].asFloat(),
		(float)vec->elements[1].asFloat()};
}
static inline auto Vector2ToValue = [](::Vector2 v) -> Value {
	std::vector<Value> elems;
	elems.reserve(2);
	elems.push_back(Value::Float(v.x));
	elems.push_back(Value::Float(v.y));
	return Value::Vector(elems);
};
static inline std::vector<::Vector2> ValueToVectorList(const Value &v, int l, int c) {
	if (v.type != ValueType::LIST)
		throw TypeError("Expected List of Vectors", l, c);
	auto *list = static_cast<ListObject *>(v.ref.get());
	std::vector<::Vector2> points;
	points.reserve(list->elements.size());
	for (const auto &el : list->elements)
		points.push_back(ValueToVector2(el, l, c));
	return points;
}

extern "C" void YlangLoadModule(std::shared_ptr<Env> env, const std::vector<std::string> &symbols) {
    auto moduleNamespace = std::make_shared<ClassObject>("Raylib");
		moduleNamespace->mro.push_back(moduleNamespace.get());
		auto define = [&](string name, NativeFunc f) {
			moduleNamespace->staticFields[name] = Value::Native(f);
			if (symbols.size() == 1 && symbols[0] == "*") {
				env->set(name, Value::Native(f), true);
				return;
			}
			for (const auto &s : symbols)
				if (s == name) {
					env->set(name, Value::Native(f), true);
					break;
				}
		};
		auto defineValue = [&](string name, Value v) {
			moduleNamespace->staticFields[name] = v;
			if (symbols.size() == 1 && symbols[0] == "*") {
				env->set(name, v, true, true);
				return;
			}
			for (const auto &s : symbols) {
				if (s == name) {
					env->set(name, v, true, true);
					break;
				}
			}
		};
		static auto colorClass = std::make_shared<ClassObject>("Color");
		static auto imageClass = std::make_shared<ClassObject>("Image");
		static auto textureClass = std::make_shared<ClassObject>("Texture2D");
		static auto renderClass = std::make_shared<ClassObject>("RenderTexture");
		static auto fontClass = std::make_shared<ClassObject>("Font");
		static auto soundClass = std::make_shared<ClassObject>("Sound");
		static auto musicClass = std::make_shared<ClassObject>("Music");
		colorClass->methods["__display__"] = ClassObject::MethodInfo{
			.func = Value::Native([&](const std::vector<Value> &args, int line, int col) -> Value {
				if (args.empty() || args[0].type != ValueType::INSTANCE)
					return Value::String("<invalid color>");
				auto *inst = static_cast<InstanceObject *>(args[0].ref.get());
				int r = inst->fields["r"].asInt();
				int g = inst->fields["g"].asInt();
				int b = inst->fields["b"].asInt();
				int a = inst->fields["a"].asInt();
				std::string info = "RGBA(" + to_string(r) + ", " + to_string(g) + ", " + to_string(b) + ", " + to_string(a) + ") " + "\033[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m██\033[0m";
				return Value::String(info);
			}),
			.access = AccessLevel::PUBLIC};
		auto ImageToValue = [&](const Image &img) -> Value {
			auto inst = std::make_shared<InstanceObject>(imageClass.get());
			inst->fields["data"] = Value::pInt(img.data);
			inst->fields["width"] = Value::Int(img.width);
			inst->fields["height"] = Value::Int(img.height);
			inst->fields["mipmaps"] = Value::Int(img.mipmaps);
			inst->fields["format"] = Value::Int(img.format);
			Value v;
			v.type = ValueType::INSTANCE;
			v.ref = inst;
			return v;
		};
		auto ValueToImage = [&](Value v, int l, int c) -> Image {
			if (v.type != ValueType::INSTANCE)
				throw TypeError("Expected Image instance", l, c);
			auto inst = static_cast<InstanceObject *>(v.ref.get());
			if (inst->klass->name != "Image")
				throw TypeError("Expected Image instance", l, c);
			Image img;
			img.data = inst->fields["data"].aspInt();
			img.width = (int)inst->fields["width"].asInt();
			img.height = (int)inst->fields["height"].asInt();
			img.mipmaps = (int)inst->fields["mipmaps"].asInt();
			img.format = (int)inst->fields["format"].asInt();
			return img;
		};
		auto TextureToValue = [&](const Texture2D &tex) -> Value {
			auto inst = std::make_shared<InstanceObject>(textureClass.get());
			inst->fields["id"] = Value::Int(tex.id); // Store OpenGL Texture ID
			inst->fields["width"] = Value::Int(tex.width);
			inst->fields["height"] = Value::Int(tex.height);
			inst->fields["mipmaps"] = Value::Int(tex.mipmaps);
			inst->fields["format"] = Value::Int(tex.format);
			Value v;
			v.type = ValueType::INSTANCE;
			v.ref = inst;
			return v;
		};
		auto ValueToTexture = [&](Value v, int l, int c) -> Texture2D {
			if (v.type != ValueType::INSTANCE)
				throw TypeError("Expected Texture2D instance", l, c);
			auto inst = static_cast<InstanceObject *>(v.ref.get());
			if (inst->klass->name != "Texture2D")
				throw TypeError("Expected Texture2D instance", l, c);
			Texture2D tex;
			tex.id = (unsigned int)inst->fields["id"].asInt();
			tex.width = (int)inst->fields["width"].asInt();
			tex.height = (int)inst->fields["height"].asInt();
			tex.mipmaps = (int)inst->fields["mipmaps"].asInt();
			tex.format = (int)inst->fields["format"].asInt();
			return tex;
		};
		auto ValueToRenderTexture = [&](Value v, int l, int c) -> RenderTexture2D {
			// Type Safety Check
			if (v.type != ValueType::INSTANCE)
				throw TypeError("Expected RenderTexture instance", l, c);
			auto inst = static_cast<InstanceObject *>(v.ref.get());
			if (inst->klass->name != "RenderTexture")
				throw TypeError("Expected RenderTexture instance", l, c);
			RenderTexture2D target;
			target.id = (unsigned int)inst->fields["id"].asInt();
			target.texture = ValueToTexture(inst->fields["texture"], l, c);
			target.depth = ValueToTexture(inst->fields["depth"], l, c);
			return target;
		};
		auto RenderTextureToValue = [&](const RenderTexture2D &target) -> Value {
			auto inst = std::make_shared<InstanceObject>(renderClass.get());
			inst->fields["id"] = Value::Int(target.id);
			inst->fields["texture"] = TextureToValue(target.texture);
			inst->fields["depth"] = TextureToValue(target.depth);
			Value v;
			v.type = ValueType::INSTANCE;
			v.ref = inst;
			return v;
		};
		auto MakeColor = [&](int r, int g, int b, int a) -> Value {
			auto inst = std::make_shared<InstanceObject>(colorClass.get());
			inst->fields["r"] = Value::Int(r);
			inst->fields["g"] = Value::Int(g);
			inst->fields["b"] = Value::Int(b);
			inst->fields["a"] = Value::Int(a);

			Value v;
			v.type = ValueType::INSTANCE;
			v.ref = inst;
			v.isConst = true; // Important: Make default colors constant!
			return v;
		};
		auto FontToValue = [&](const Font &font) -> Value {
			auto inst = std::make_shared<InstanceObject>(fontClass.get());
			inst->fields["baseSize"] = Value::Int(font.baseSize);
			inst->fields["glyphCount"] = Value::Int(font.glyphCount);
			inst->fields["glyphPadding"] = Value::Int(font.glyphPadding);
			inst->fields["texture"] = TextureToValue(font.texture);
			inst->fields["recs"] = Value::pInt(font.recs);
			inst->fields["glyphs"] = Value::pInt(font.glyphs);
			Value v;
			v.type = ValueType::INSTANCE;
			v.ref = inst;
			return v;
		};
		auto ValueToFont = [&](Value v, int l, int c) -> Font {
			if (v.type != ValueType::INSTANCE)
				throw TypeError("Expected Font instance", l, c);
			auto inst = static_cast<InstanceObject *>(v.ref.get());
			if (inst->klass->name != "Font")
				throw TypeError("Expected Font instance", l, c);
			Font font;
			font.baseSize = (int)inst->fields["baseSize"].asInt();
			font.glyphCount = (int)inst->fields["glyphCount"].asInt();
			font.glyphPadding = (int)inst->fields["glyphPadding"].asInt();
			font.texture = ValueToTexture(inst->fields["texture"], l, c);
			font.recs = (Rectangle *)inst->fields["recs"].aspInt();
			font.glyphs = (GlyphInfo *)inst->fields["glyphs"].aspInt();
			return font;
		};
		auto SoundToValue = [&](Sound s) -> Value {
			auto inst = std::make_shared<InstanceObject>(soundClass.get());
			inst->fields["buffer"] =
				Value::Int((long long)(uintptr_t)s.stream.buffer);
			inst->fields["processor"] =
				Value::Int((long long)(uintptr_t)s.stream.processor);
			inst->fields["sampleRate"] = Value::Int(s.stream.sampleRate);
			inst->fields["sampleSize"] = Value::Int(s.stream.sampleSize);
			inst->fields["channels"] = Value::Int(s.stream.channels);
			inst->fields["frameCount"] = Value::Int(s.frameCount);
			Value v;
			v.type = ValueType::INSTANCE;
			v.ref = inst;
			return v;
		};
		auto MusicToValue = [&](Music m) -> Value {
			auto inst = std::make_shared<InstanceObject>(musicClass.get());
			inst->fields["buffer"] =
				Value::Int((long long)(uintptr_t)m.stream.buffer);
			inst->fields["processor"] =
				Value::Int((long long)(uintptr_t)m.stream.processor);
			inst->fields["sampleRate"] = Value::Int(m.stream.sampleRate);
			inst->fields["sampleSize"] = Value::Int(m.stream.sampleSize);
			inst->fields["channels"] = Value::Int(m.stream.channels);
			inst->fields["frameCount"] = Value::Int(m.frameCount);
			inst->fields["looping"] = Value::Bool(m.looping);
			inst->fields["ctxType"] = Value::Int(m.ctxType);
			inst->fields["ctxData"] = Value::Int((long long)(uintptr_t)m.ctxData);
			Value v;
			v.type = ValueType::INSTANCE;
			v.ref = inst;
			return v;
		};
		auto ValueToSound = [&](Value v, int l, int c) -> Sound {
			if (v.type != ValueType::INSTANCE)
				throw ArgumentError("Expected Sound object", l, c);
			auto *inst = static_cast<InstanceObject *>(v.ref.get());
			Sound s;
			s.stream.buffer =
				(rAudioBuffer *)(uintptr_t)inst->fields["buffer"].asInt();
			s.stream.processor =
				(rAudioProcessor *)(uintptr_t)inst->fields["processor"].asInt();
			s.stream.sampleRate = (unsigned int)inst->fields["sampleRate"].asInt();
			s.stream.sampleSize = (unsigned int)inst->fields["sampleSize"].asInt();
			s.stream.channels = (unsigned int)inst->fields["channels"].asInt();
			s.frameCount = (unsigned int)inst->fields["frameCount"].asInt();
			return s;
		};
		auto ValueToMusic = [&](Value v, int l, int c) -> Music {
			if (v.type != ValueType::INSTANCE)
				throw ArgumentError("Expected Music object", l, c);
			auto *inst = static_cast<InstanceObject *>(v.ref.get());
			Music m;
			m.stream.buffer =
				(rAudioBuffer *)(uintptr_t)inst->fields["buffer"].asInt();
			m.stream.processor =
				(rAudioProcessor *)(uintptr_t)inst->fields["processor"].asInt();
			m.stream.sampleRate = (unsigned int)inst->fields["sampleRate"].asInt();
			m.stream.sampleSize = (unsigned int)inst->fields["sampleSize"].asInt();
			m.stream.channels = (unsigned int)inst->fields["channels"].asInt();
			m.frameCount = (unsigned int)inst->fields["frameCount"].asInt();
			m.looping = inst->fields["looping"].asBool();
			m.ctxType = (int)inst->fields["ctxType"].asInt();
			m.ctxData = (void *)(uintptr_t)inst->fields["ctxData"].asInt();
			return m;
		};
		defineValue("LIGHTGRAY", MakeColor(200, 200, 200, 255));
		defineValue("GRAY", MakeColor(130, 130, 130, 255));
		defineValue("DARKGRAY", MakeColor(80, 80, 80, 255));
		defineValue("YELLOW", MakeColor(253, 249, 0, 255));
		defineValue("GOLD", MakeColor(255, 203, 0, 255));
		defineValue("ORANGE", MakeColor(255, 161, 0, 255));
		defineValue("PINK", MakeColor(255, 109, 194, 255));
		defineValue("RED", MakeColor(230, 41, 55, 255));
		defineValue("MAROON", MakeColor(190, 33, 55, 255));
		defineValue("GREEN", MakeColor(0, 228, 48, 255));
		defineValue("LIME", MakeColor(0, 158, 47, 255));
		defineValue("DARKGREEN", MakeColor(0, 117, 44, 255));
		defineValue("SKYBLUE", MakeColor(102, 191, 255, 255));
		defineValue("BLUE", MakeColor(0, 121, 241, 255));
		defineValue("DARKBLUE", MakeColor(0, 82, 172, 255));
		defineValue("PURPLE", MakeColor(200, 122, 255, 255));
		defineValue("VIOLET", MakeColor(135, 60, 190, 255));
		defineValue("DARKPURPLE", MakeColor(112, 31, 126, 255));
		defineValue("BEIGE", MakeColor(211, 176, 131, 255));
		defineValue("BROWN", MakeColor(127, 106, 79, 255));
		defineValue("DARKBROWN", MakeColor(76, 63, 47, 255));
		defineValue("WHITE", MakeColor(255, 255, 255, 255));
		defineValue("BLACK", MakeColor(0, 0, 0, 255));
		defineValue("BLANK", MakeColor(0, 0, 0, 0));
		defineValue("MAGENTA", MakeColor(255, 0, 255, 255));
		defineValue("CYAN", MakeColor(0, 255, 255, 255));
		defineValue("RAYWHITE", MakeColor(245, 245, 245, 255));
		defineValue("TEXTURE_FILTER_POINT", Value::Int(0));
		defineValue("TEXTURE_FILTER_BILINEAR", Value::Int(1));
		defineValue("TEXTURE_FILTER_TRILINEAR", Value::Int(2));
		defineValue("TEXTURE_FILTER_ANISOTROPIC_4X", Value::Int(3));
		defineValue("TEXTURE_FILTER_ANISOTROPIC_8X", Value::Int(4));
		defineValue("TEXTURE_FILTER_ANISOTROPIC_16X", Value::Int(5));
		defineValue("TEXTURE_WRAP_REPEAT", Value::Int(0));
		defineValue("TEXTURE_WRAP_CLAMP", Value::Int(1));
		defineValue("TEXTURE_WRAP_MIRROR_REPEAT", Value::Int(2));
		defineValue("TEXTURE_WRAP_MIRROR_CLAMP", Value::Int(3));
		defineValue("FLAG_VSYNC_HINT", Value::Int(64));
		defineValue("FLAG_FULLSCREEN_MODE", Value::Int(2));
		defineValue("FLAG_WINDOW_RESIZABLE", Value::Int(4));
		defineValue("FLAG_WINDOW_UNDECORATED", Value::Int(8));
		defineValue("FLAG_WINDOW_TRANSPARENT", Value::Int(16));
		defineValue("FLAG_WINDOW_HIDDEN", Value::Int(128));
		defineValue("FLAG_WINDOW_MINIMIZED", Value::Int(512));
		defineValue("FLAG_WINDOW_MAXIMIZED", Value::Int(1024));
		defineValue("FLAG_WINDOW_UNFOCUSED", Value::Int(2048));
		defineValue("FLAG_WINDOW_TOPMOST", Value::Int(4096));
		defineValue("FLAG_WINDOW_ALWAYS_RUN", Value::Int(256));
		defineValue("FLAG_WINDOW_HIGHDPI", Value::Int(8192));
		defineValue("FLAG_WINDOW_MOUSE_PASSTHROUGH", Value::Int(16384));
		defineValue("FLAG_BORDERLESS_WINDOWED_MODE", Value::Int(32768));
		defineValue("FLAG_MSAA_4X_HINT", Value::Int(32));
		defineValue("FLAG_INTERLACED_HINT", Value::Int(65536));
		defineValue("PIXELFORMAT_UNCOMPRESSED_GRAYSCALE", Value::Int(1));
		defineValue("PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA", Value::Int(2));
		defineValue("PIXELFORMAT_UNCOMPRESSED_R5G6B5", Value::Int(3));
		defineValue("PIXELFORMAT_UNCOMPRESSED_R8G8B8", Value::Int(4));
		defineValue("PIXELFORMAT_UNCOMPRESSED_R5G5B5A1", Value::Int(5));
		defineValue("PIXELFORMAT_UNCOMPRESSED_R4G4B4A4", Value::Int(6));
		defineValue("PIXELFORMAT_UNCOMPRESSED_R8G8B8A8", Value::Int(7));
		defineValue("PIXELFORMAT_UNCOMPRESSED_R32", Value::Int(8));
		defineValue("PIXELFORMAT_UNCOMPRESSED_R32G32B32", Value::Int(9));
		defineValue("PIXELFORMAT_UNCOMPRESSED_R32G32B32A32", Value::Int(10));
		defineValue("PIXELFORMAT_UNCOMPRESSED_R16", Value::Int(11));
		defineValue("PIXELFORMAT_UNCOMPRESSED_R16G16B16", Value::Int(12));
		defineValue("PIXELFORMAT_UNCOMPRESSED_R16G16B16A16", Value::Int(13));
		defineValue("PIXELFORMAT_COMPRESSED_DXT1_RGB", Value::Int(14));
		defineValue("PIXELFORMAT_COMPRESSED_DXT1_RGBA", Value::Int(15));
		defineValue("PIXELFORMAT_COMPRESSED_DXT3_RGBA", Value::Int(16));
		defineValue("PIXELFORMAT_COMPRESSED_DXT5_RGBA", Value::Int(17));
		defineValue("PIXELFORMAT_COMPRESSED_ETC1_RGB", Value::Int(18));
		defineValue("PIXELFORMAT_COMPRESSED_ETC2_RGB", Value::Int(19));
		defineValue("PIXELFORMAT_COMPRESSED_ETC2_EAC_RGBA", Value::Int(20));
		defineValue("PIXELFORMAT_COMPRESSED_PVRT_RGB", Value::Int(21));
		defineValue("PIXELFORMAT_COMPRESSED_PVRT_RGBA", Value::Int(22));
		defineValue("PIXELFORMAT_COMPRESSED_ASTC_4x4_RGBA", Value::Int(23));
		defineValue("PIXELFORMAT_COMPRESSED_ASTC_8x8_RGBA", Value::Int(24));
		defineValue("BLEND_ALPHA", Value::Int(0));
		defineValue("BLEND_ADDITIVE", Value::Int(1));
		defineValue("BLEND_MULTIPLIED", Value::Int(2));
		defineValue("BLEND_ADD_COLORS", Value::Int(3));
		defineValue("BLEND_SUBTRACT_COLORS", Value::Int(4));
		defineValue("BLEND_ALPHA_PREMULTIPLY", Value::Int(5));
		defineValue("BLEND_CUSTOM", Value::Int(6));
		defineValue("BLEND_CUSTOM_SEPARATE", Value::Int(7));
		defineValue("KEY_NULL", Value::Int(0));
		defineValue("KEY_APOSTROPHE", Value::Int(39));
		defineValue("KEY_COMMA", Value::Int(44));
		defineValue("KEY_MINUS", Value::Int(45));
		defineValue("KEY_PERIOD", Value::Int(46));
		defineValue("KEY_SLASH", Value::Int(47));
		defineValue("KEY_ZERO", Value::Int(48));
		defineValue("KEY_ONE", Value::Int(49));
		defineValue("KEY_TWO", Value::Int(50));
		defineValue("KEY_THREE", Value::Int(51));
		defineValue("KEY_FOUR", Value::Int(52));
		defineValue("KEY_FIVE", Value::Int(53));
		defineValue("KEY_SIX", Value::Int(54));
		defineValue("KEY_SEVEN", Value::Int(55));
		defineValue("KEY_EIGHT", Value::Int(56));
		defineValue("KEY_NINE", Value::Int(57));
		defineValue("KEY_SEMICOLON", Value::Int(59));
		defineValue("KEY_EQUAL", Value::Int(61));
		defineValue("KEY_A", Value::Int(65));
		defineValue("KEY_B", Value::Int(66));
		defineValue("KEY_C", Value::Int(67));
		defineValue("KEY_D", Value::Int(68));
		defineValue("KEY_E", Value::Int(69));
		defineValue("KEY_F", Value::Int(70));
		defineValue("KEY_G", Value::Int(71));
		defineValue("KEY_H", Value::Int(72));
		defineValue("KEY_I", Value::Int(73));
		defineValue("KEY_J", Value::Int(74));
		defineValue("KEY_K", Value::Int(75));
		defineValue("KEY_L", Value::Int(76));
		defineValue("KEY_M", Value::Int(77));
		defineValue("KEY_N", Value::Int(78));
		defineValue("KEY_O", Value::Int(79));
		defineValue("KEY_P", Value::Int(80));
		defineValue("KEY_Q", Value::Int(81));
		defineValue("KEY_R", Value::Int(82));
		defineValue("KEY_S", Value::Int(83));
		defineValue("KEY_T", Value::Int(84));
		defineValue("KEY_U", Value::Int(85));
		defineValue("KEY_V", Value::Int(86));
		defineValue("KEY_W", Value::Int(87));
		defineValue("KEY_X", Value::Int(88));
		defineValue("KEY_Y", Value::Int(89));
		defineValue("KEY_Z", Value::Int(90));
		defineValue("KEY_LEFT_BRACKET", Value::Int(91));
		defineValue("KEY_BACKSLASH", Value::Int(92));
		defineValue("KEY_RIGHT_BRACKET", Value::Int(93));
		defineValue("KEY_GRAVE", Value::Int(96));
		defineValue("KEY_SPACE", Value::Int(32));
		defineValue("KEY_ESCAPE", Value::Int(256));
		defineValue("KEY_ENTER", Value::Int(257));
		defineValue("KEY_TAB", Value::Int(258));
		defineValue("KEY_BACKSPACE", Value::Int(259));
		defineValue("KEY_INSERT", Value::Int(260));
		defineValue("KEY_DELETE", Value::Int(261));
		defineValue("KEY_RIGHT", Value::Int(262));
		defineValue("KEY_LEFT", Value::Int(263));
		defineValue("KEY_DOWN", Value::Int(264));
		defineValue("KEY_UP", Value::Int(265));
		defineValue("KEY_PAGE_UP", Value::Int(266));
		defineValue("KEY_PAGE_DOWN", Value::Int(267));
		defineValue("KEY_HOME", Value::Int(268));
		defineValue("KEY_END", Value::Int(269));
		defineValue("KEY_CAPS_LOCK", Value::Int(280));
		defineValue("KEY_SCROLL_LOCK", Value::Int(281));
		defineValue("KEY_NUM_LOCK", Value::Int(282));
		defineValue("KEY_PRINT_SCREEN", Value::Int(283));
		defineValue("KEY_PAUSE", Value::Int(284));
		defineValue("KEY_F1", Value::Int(290));
		defineValue("KEY_F2", Value::Int(291));
		defineValue("KEY_F3", Value::Int(292));
		defineValue("KEY_F4", Value::Int(293));
		defineValue("KEY_F5", Value::Int(294));
		defineValue("KEY_F6", Value::Int(295));
		defineValue("KEY_F7", Value::Int(296));
		defineValue("KEY_F8", Value::Int(297));
		defineValue("KEY_F9", Value::Int(298));
		defineValue("KEY_F10", Value::Int(299));
		defineValue("KEY_F11", Value::Int(300));
		defineValue("KEY_F12", Value::Int(301));
		defineValue("KEY_LEFT_SHIFT", Value::Int(340));
		defineValue("KEY_LEFT_CONTROL", Value::Int(341));
		defineValue("KEY_LEFT_ALT", Value::Int(342));
		defineValue("KEY_LEFT_SUPER", Value::Int(343));
		defineValue("KEY_RIGHT_SHIFT", Value::Int(344));
		defineValue("KEY_RIGHT_CONTROL", Value::Int(345));
		defineValue("KEY_RIGHT_ALT", Value::Int(346));
		defineValue("KEY_RIGHT_SUPER", Value::Int(347));
		defineValue("KEY_KB_MENU", Value::Int(348));
		defineValue("KEY_KP_0", Value::Int(320));
		defineValue("KEY_KP_1", Value::Int(321));
		defineValue("KEY_KP_2", Value::Int(322));
		defineValue("KEY_KP_3", Value::Int(323));
		defineValue("KEY_KP_4", Value::Int(324));
		defineValue("KEY_KP_5", Value::Int(325));
		defineValue("KEY_KP_6", Value::Int(326));
		defineValue("KEY_KP_7", Value::Int(327));
		defineValue("KEY_KP_8", Value::Int(328));
		defineValue("KEY_KP_9", Value::Int(329));
		defineValue("KEY_KP_DECIMAL", Value::Int(330));
		defineValue("KEY_KP_DIVIDE", Value::Int(331));
		defineValue("KEY_KP_MULTIPLY", Value::Int(332));
		defineValue("KEY_KP_SUBTRACT", Value::Int(333));
		defineValue("KEY_KP_ADD", Value::Int(334));
		defineValue("KEY_KP_ENTER", Value::Int(335));
		defineValue("KEY_KP_EQUAL", Value::Int(336));
		defineValue("GAMEPAD_BUTTON_UNKNOWN", Value::Int(0));
		defineValue("GAMEPAD_BUTTON_LEFT_FACE_UP", Value::Int(1));
		defineValue("GAMEPAD_BUTTON_LEFT_FACE_RIGHT", Value::Int(2));
		defineValue("GAMEPAD_BUTTON_LEFT_FACE_DOWN", Value::Int(3));
		defineValue("GAMEPAD_BUTTON_LEFT_FACE_LEFT", Value::Int(4));
		defineValue("GAMEPAD_BUTTON_RIGHT_FACE_UP", Value::Int(5));
		defineValue("GAMEPAD_BUTTON_RIGHT_FACE_RIGHT", Value::Int(6));
		defineValue("GAMEPAD_BUTTON_RIGHT_FACE_DOWN", Value::Int(7));
		defineValue("GAMEPAD_BUTTON_RIGHT_FACE_LEFT", Value::Int(8));
		defineValue("GAMEPAD_BUTTON_LEFT_TRIGGER_1", Value::Int(9));
		defineValue("GAMEPAD_BUTTON_LEFT_TRIGGER_2", Value::Int(10));
		defineValue("GAMEPAD_BUTTON_RIGHT_TRIGGER_1", Value::Int(11));
		defineValue("GAMEPAD_BUTTON_RIGHT_TRIGGER_2", Value::Int(12));
		defineValue("GAMEPAD_BUTTON_MIDDLE_LEFT", Value::Int(13));
		defineValue("GAMEPAD_BUTTON_MIDDLE", Value::Int(14));
		defineValue("GAMEPAD_BUTTON_MIDDLE_RIGHT", Value::Int(15));
		defineValue("GAMEPAD_BUTTON_LEFT_THUMB", Value::Int(16));
		defineValue("GAMEPAD_BUTTON_RIGHT_THUMB", Value::Int(17));

		define("Color", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4)
				throw ArgumentError("Color(r, g, b, a)", l, c);
			auto inst = std::make_shared<InstanceObject>(colorClass.get());
			inst->fields["r"] = args[0];
			inst->fields["g"] = args[1];
			inst->fields["b"] = args[2];
			inst->fields["a"] = args[3];
			Value v;
			v.type = ValueType::INSTANCE;
			v.ref = inst;
			return v;
		});
		define("Rectangle", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4)
				throw ArgumentError("Rectangle(x, y, w, h)", l, c);
			static auto rectClass = std::make_shared<ClassObject>("Rectangle");
			auto inst = std::make_shared<InstanceObject>(rectClass.get());
			inst->fields["x"] = args[0];
			inst->fields["y"] = args[1];
			inst->fields["width"] = args[2];
			inst->fields["height"] = args[3];
			Value v;
			v.type = ValueType::INSTANCE;
			v.ref = inst;
			return v;
		});
		define("Font", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 6)
				throw ArgumentError(
					"Font(baseSize, glyphCount, glyphPadding, texture, recs_ptr, "
					"glyphs_ptr)",
					l, c);
			auto inst = std::make_shared<InstanceObject>(fontClass.get());
			inst->fields["baseSize"] = args[0];
			inst->fields["glyphCount"] = args[1];
			inst->fields["glyphPadding"] = args[2];
			inst->fields["texture"] = args[3];
			inst->fields["recs"] = args[4];
			inst->fields["glyphs"] = args[5];
			Value v;
			v.type = ValueType::INSTANCE;
			v.ref = inst;
			return v;
		});
		define("FadeColor", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError(
					"Fade() takes two arguments, (color, fadeAmount)", l, c);
			Color newColor = Fade(ValueToColor(args[0], l, c), args[1].asFloat());
			auto v = MakeColor(newColor.r, newColor.g, newColor.b, newColor.a);
			v.isConst = false;
			return v;
		});
		define("ColorToGrayFast", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("ColorToGrayFast() takes one argument, (color)",
					l, c);
			Color newColor = ValueToColor(args[0], l, c);
			uint8_t G = static_cast<uint8_t>(
				floorf((newColor.r + newColor.g + newColor.b) / 3.0F));
			auto v = MakeColor(G, G, G, newColor.a);
			v.isConst = false;
			return v;
		});
		define("ColorToGray", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("ColorToGray() takes one argument, (color)", l,
					c);
			Color newColor = ValueToColor(args[0], l, c);
			float R_linear = powf(newColor.r / 255.0F, 2.2F);
			float G_linear = powf(newColor.g / 255.0F, 2.2F);
			float B_linear = powf(newColor.r / 255.0F, 2.2F);
			float GRAY_linear =
				(0.2126F * R_linear) + (0.7152F * G_linear) + (0.0722F * B_linear);
			uint8_t G = static_cast<uint8_t>(
				floorf(255.0F * powf(GRAY_linear, 1.0F / 2.2F)));
			auto v = MakeColor(G, G, G, newColor.a);
			v.isConst = false;
			return v;
		});
		define("InitWindow", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3)
				throw ArgumentError(
					"InitWindow() takes three arguments, (width, height, text)", l,
					c);
			InitWindow((int)args[0].asInt(), (int)args[1].asInt(),
				args[2].asString().c_str());
			return Value::None();
		});
		define("ToggleFullscreen", [=](const vector<Value> &args, int l, int c) {
			if (!args.empty())
				throw ArgumentError("ToggleFullscreen() takes no arguments", l, c);
			ToggleFullscreen();
			return Value::None();
		});
		define("CloseWindow", [=](const vector<Value> &args, int l, int c) {
			if (!args.empty())
				throw ArgumentError("CloseWindow() takes no arguments", l, c);
			CloseWindow();
			return Value::None();
		});
		define("WindowShouldClose", [=](const vector<Value> &args, int l, int c) {
			if (!args.empty())
				throw ArgumentError("WindowShouldClose() takes no arguments", l, c);
			return Value::Bool((bool)WindowShouldClose());
		});
		define("BeginDrawing", [=](const vector<Value> &args, int l, int c) {
			BeginDrawing();
			return Value::None();
		});
		define("BeginBlendMode", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError(
					"BeginBlendMode() takes only one argument (mode)", l, c);
			BeginBlendMode(args[0].asInt());
			return Value::None();
		});
		define("EndBlendMode", [=](const vector<Value> &args, int l, int c) {
			EndBlendMode();
			return Value::None();
		});
		define("EndDrawing", [=](const vector<Value> &args, int l, int c) {
			EndDrawing();
			return Value::None();
		});
		define("SetConfigFlags", [](const std::vector<Value> &args, int l, int c) {
			if (args.size() < 1) 
				throw ArgumentError("SetConfigFlags(flags...)", l, c);
			unsigned int flags = 0;
			for (const auto& f : args) flags |= static_cast<unsigned int>(f.asInt());
			SetConfigFlags(flags);
			return Value::None();
		});
		define("SetTextureFilter", [=](const std::vector<Value> &args, int l, int c) {
			if (args.size() < 2) 
				throw ArgumentError("SetTextureFilter(texture, flags...)", l, c);
			unsigned int flags = 0;
			for (auto f = args.begin() + 1; f < args.end(); f++) flags |= static_cast<unsigned int>(f->asInt());
			auto t = ValueToTexture(args[0], l, c);
			SetTextureFilter(t, flags);
			return Value::None();
		});
		define("SetWindowPosition", [](const std::vector<Value> &args, int l, int c) {
			if (args.size() != 2) 
				throw ArgumentError("SetWindowPosition(x, y)", l, c);
			SetWindowPosition(args[0].asInt(), args[1].asInt());
			return Value::None();
		});
		define("SetWindowPositionV", [](const std::vector<Value> &args, int l, int c) {
			if (args.size() != 1) 
				throw ArgumentError("SetWindowPositionV(VectorXY)", l, c);
			::Vector2 pos = ValueToVector2(args[0], l, c);
			SetWindowPosition(pos.x, pos.y);
			return Value::None();
		});
		define("GetMonitorWidth", [](const std::vector<Value> &args, int l, int c) {
			if (args.size() != 1) 
				throw ArgumentError("GetMonitorWidth(monitorNumber)", l, c);
			return Value::Int(GetMonitorWidth(args[0].asInt()));
		});
		define("GetMonitorHeight", [](const std::vector<Value> &args, int l, int c) {
			if (args.size() != 1) 
				throw ArgumentError("GetMonitorHeight(monitorNumber)", l, c);
			return Value::Int(GetMonitorHeight(args[0].asInt()));
		});
		define("GetWindowScaleV", [](const std::vector<Value> &args, int l, int c) {
			if (args.size() != 0) 
				throw ArgumentError("GetWindowScaleV()", l, c);
			::Vector2 scale = GetWindowScaleDPI();
			return Vector2ToValue(scale); 
		});
		define("GetWindowScaleF", [](const std::vector<Value> &args, int l, int c) {
			if (args.size() != 0) 
				throw ArgumentError("GetWindowScaleF()", l, c);
			::Vector2 scale = GetWindowScaleDPI();
			return Value::Float((double)scale.x); 
		});
		define("LoadRenderTexture", [&](const std::vector<Value> &args, int l, int c) {
			if (args.size() != 2) 
				throw ArgumentError("LoadRenderTexture(width, height)", l, c);
			int width = (int)args[0].asInt();
			int height = (int)args[1].asInt();
			RenderTexture2D target = LoadRenderTexture(width, height);
			return RenderTextureToValue(target);
		});
		define("BeginTextureMode", [&](const std::vector<Value> &args, int l, int c) {
			if (args.size() != 1) 
				throw ArgumentError("BeginTextureMode(target)", l, c);
			RenderTexture2D target = ValueToRenderTexture(args[0], l, c);
			BeginTextureMode(target);
			return Value::None();
		});
		define("EndTextureMode", [](const std::vector<Value> &args, int l, int c) {
			if (args.size() != 0) 
				throw ArgumentError("EndTextureMode()", l, c);
			EndTextureMode();
			return Value::None();
		});
		define("UnloadRenderTexture", [&](const std::vector<Value> &args, int l, int c) {
			if (args.size() != 1) 
				throw ArgumentError("UnloadRenderTexture(target)", l, c);
			RenderTexture2D target = ValueToRenderTexture(args[0], l, c);
			UnloadRenderTexture(target);
			return Value::None();
		});
		define("ClearBackground", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("ClearBackground(Color)", l, c);
			ClearBackground(ValueToColor(args[0], l, c));
			return Value::None();
		});
		define("GetFrameTime", [=](const vector<Value> &args, int l, int c) {
			if (!args.empty())
				throw ArgumentError("GetFrameTime()", l, c);
			return Value::Float(GetFrameTime());
		});
		define("SetTargetFPS", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("SetTargetFPS(fps)", l, c);
			SetTargetFPS((int)args[0].asInt());
			return Value::None();
		});
		define("DrawFPS", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("DrawFPS(x, y)", l, c);
			DrawFPS((int)args[0].asInt(), (int)args[1].asInt());
			return Value::None();
		});
		define("GetFontDefault", [=](const vector<Value> &args, int l, int c) {
			if (!args.empty())
				throw ArgumentError("GetFontDefault() takes no arguments", l, c);
			return FontToValue(GetFontDefault());
		});
		define("LoadFont", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("LoadFont(fileName)", l, c);
			Font font = LoadFont(args[0].asString().c_str());
			return FontToValue(font);
		});
		define("LoadFontEx", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3)
				throw ArgumentError("LoadFontEx(fileName, fontSize, fontChars_ptr)",
					l, c);
			// Usually pass Value::Omit() or Value::None() for the 3rd arg to load
			// default ASCII
			int *chars =
				args[2].type == ValueType::NONE ? nullptr : (int *)args[2].aspInt();
			Font font = LoadFontEx(args[0].asString().c_str(),
				(int)args[1].asInt(), chars, 0);
			return FontToValue(font);
		});
		define("UnloadFont", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("UnloadFont(font)", l, c);
			UnloadFont(ValueToFont(args[0], l, c));
			return Value::None();
		});
		define("DrawTextEx", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 6)
				throw ArgumentError(
					"DrawTextEx(font, text, positionVec, fontSize, spacing, "
					"tintColor)",
					l, c);
			DrawTextEx(ValueToFont(args[0], l, c), args[1].asString().c_str(),
				ValueToVector2(args[2], l, c), (float)args[3].asFloat(),
				(float)args[4].asFloat(), ValueToColor(args[5], l, c));
			return Value::None();
		});
		define("DrawTextPro", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 8)
				throw ArgumentError(
					"DrawTextPro(font, text, posVec, originVec, rotation, "
					"fontSize, "
					"spacing, tintColor)",
					l, c);
			DrawTextPro(ValueToFont(args[0], l, c), args[1].asString().c_str(),
				ValueToVector2(args[2], l, c),
				ValueToVector2(args[3], l, c), (float)args[4].asFloat(),
				(float)args[5].asFloat(), (float)args[6].asFloat(),
				ValueToColor(args[7], l, c));
			return Value::None();
		});
		define("MeasureTextEx", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4)
				throw ArgumentError("MeasureTextEx(font, text, fontSize, spacing)",
					l, c);
			::Vector2 size = MeasureTextEx(
				ValueToFont(args[0], l, c), args[1].asString().c_str(),
				(float)args[2].asFloat(), (float)args[3].asFloat());
			return Vector2ToValue(size);
		});
		define("DrawText", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5)
				throw ArgumentError("DrawText(str, x, y, size, color)", l, c);
			DrawText(args[0].asString().c_str(), (int)args[1].asInt(),
				(int)args[2].asInt(), (int)args[3].asInt(),
				ValueToColor(args[4], l, c));
			return Value::None();
		});
		define("DrawPixel", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 3)
				throw ArgumentError("DrawPixel(x, y, color)", l, c);
			DrawPixel((int)args[0].asInt(), (int)args[1].asInt(),
				ValueToColor(args[2], l, c));
			return Value::None();
		});
		define("DrawPixelV", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("DrawPixelV(vec, color)", l, c);
			DrawPixelV(ValueToVector2(args[0], l, c), ValueToColor(args[1], l, c));
			return Value::None();
		});
		define("DrawLine", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 5)
				throw ArgumentError("DrawLine(x1, y1, x2, y2, color)", l, c);
			DrawLine((int)args[0].asInt(), (int)args[1].asInt(),
				(int)args[2].asInt(), (int)args[3].asInt(),
				ValueToColor(args[4], l, c));
			return Value::None();
		});
		define("DrawLineV", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 3)
				throw ArgumentError("DrawLineV(vec1, vec2, color)", l, c);
			DrawLineV(ValueToVector2(args[0], l, c), ValueToVector2(args[1], l, c),
				ValueToColor(args[2], l, c));
			return Value::None();
		});
		define("DrawLineEx", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 4)
				throw ArgumentError("DrawLineEx(vec1, vec2, thick, color)", l, c);
			DrawLineEx(ValueToVector2(args[0], l, c),
				ValueToVector2(args[1], l, c), (float)args[2].asFloat(),
				ValueToColor(args[3], l, c));
			return Value::None();
		});
		define("DrawLineBezier", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 4)
				throw ArgumentError("DrawLineBezier(vec1, vec2, thick, color)", l,
					c);
			DrawLineBezier(ValueToVector2(args[0], l, c),
				ValueToVector2(args[1], l, c), (float)args[2].asFloat(),
				ValueToColor(args[3], l, c));
			return Value::None();
		});
		define("DrawLineStrip", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("DrawLineStrip(pointList, color)", l, c);
			auto points = ValueToVectorList(args[0], l, c);
			DrawLineStrip(points.data(), (int)points.size(),
				ValueToColor(args[1], l, c));
			return Value::None();
		});
		define("DrawCircle", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 4)
				throw ArgumentError("DrawCircle(x, y, radius, color)", l, c);
			DrawCircle((int)args[0].asInt(), (int)args[1].asInt(),
				(float)args[2].asFloat(), ValueToColor(args[3], l, c));
			return Value::None();
		});
		define("DrawCircleV", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 3)
				throw ArgumentError("DrawCircleV(centerVec, radius, color)", l, c);
			DrawCircleV(ValueToVector2(args[0], l, c), (float)args[1].asFloat(),
				ValueToColor(args[2], l, c));
			return Value::None();
		});
		define("DrawCircleLines", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 4)
				throw ArgumentError("DrawCircleLines(x, y, radius, color)", l, c);
			DrawCircleLines((int)args[0].asInt(), (int)args[1].asInt(),
				(float)args[2].asFloat(), ValueToColor(args[3], l, c));
			return Value::None();
		});
		define("DrawCircleGradient", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 5)
				throw ArgumentError(
					"DrawCircleGradient(x, y, radius, inner, outer)", l, c);
			DrawCircleGradient((int)args[0].asInt(), (int)args[1].asInt(),
				(float)args[2].asFloat(),
				ValueToColor(args[3], l, c),
				ValueToColor(args[4], l, c));
			return Value::None();
		});
		define("DrawCircleSector", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 6)
				throw ArgumentError(
					"DrawCircleSector(center, radius, start, end, segs, color)", l,
					c);
			DrawCircleSector(ValueToVector2(args[0], l, c),
				(float)args[1].asFloat(), (float)args[2].asFloat(),
				(float)args[3].asFloat(), (int)args[4].asInt(),
				ValueToColor(args[5], l, c));
			return Value::None();
		});
		define("DrawEllipse", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 5)
				throw ArgumentError("DrawEllipse(x, y, radH, radV, color)", l, c);
			DrawEllipse((int)args[0].asInt(), (int)args[1].asInt(),
				(float)args[2].asFloat(), (float)args[3].asFloat(),
				ValueToColor(args[4], l, c));
			return Value::None();
		});
		define("DrawRing", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 7)
				throw ArgumentError(
					"DrawRing(center, inner, outer, start, end, segs, color)", l,
					c);
			DrawRing(ValueToVector2(args[0], l, c), (float)args[1].asFloat(),
				(float)args[2].asFloat(), (float)args[3].asFloat(),
				(float)args[4].asFloat(), (int)args[5].asInt(),
				ValueToColor(args[6], l, c));
			return Value::None();
		});
		define("DrawRectangle", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 5)
				throw ArgumentError("DrawRectangle(x, y, w, h, color)", l, c);
			DrawRectangle((int)args[0].asInt(), (int)args[1].asInt(),
				(int)args[2].asInt(), (int)args[3].asInt(),
				ValueToColor(args[4], l, c));
			return Value::None();
		});
		define("DrawRectangleV", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 3)
				throw ArgumentError("DrawRectangleV(posVec, sizeVec, color)", l, c);
			DrawRectangleV(ValueToVector2(args[0], l, c),
				ValueToVector2(args[1], l, c),
				ValueToColor(args[2], l, c));
			return Value::None();
		});
		define("DrawRectangleRec", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("DrawRectangleRec(rect, color)", l, c);
			DrawRectangleRec(ValueToRect(args[0], l, c),
				ValueToColor(args[1], l, c));
			return Value::None();
		});
		define("DrawRectanglePro", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 4)
				throw ArgumentError("DrawRectanglePro(rect, originVec, rot, color)",
					l, c);
			DrawRectanglePro(
				ValueToRect(args[0], l, c), ValueToVector2(args[1], l, c),
				(float)args[2].asFloat(), ValueToColor(args[3], l, c));
			return Value::None();
		});
		define("DrawRectangleGradientV", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 6)
				throw ArgumentError(
					"DrawRectangleGradientV(x, y, w, h, topCol, botCol)", l, c);
			DrawRectangleGradientV((int)args[0].asInt(), (int)args[1].asInt(),
				(int)args[2].asInt(), (int)args[3].asInt(),
				ValueToColor(args[4], l, c),
				ValueToColor(args[5], l, c));
			return Value::None();
		});
		define("DrawRectangleRounded", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 4)
				throw ArgumentError(
					"DrawRectangleRounded(rect, roundness, segs, color)", l, c);
			DrawRectangleRounded(
				ValueToRect(args[0], l, c), (float)args[1].asFloat(),
				(int)args[2].asInt(), ValueToColor(args[3], l, c));
			return Value::None();
		});
		define("DrawTriangle", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 4)
				throw ArgumentError("DrawTriangle(v1, v2, v3, color)", l, c);
			DrawTriangle(
				ValueToVector2(args[0], l, c), ValueToVector2(args[1], l, c),
				ValueToVector2(args[2], l, c), ValueToColor(args[3], l, c));
			return Value::None();
		});
		define("DrawTriangleFan", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("DrawTriangleFan(pointList, color)", l, c);
			auto points = ValueToVectorList(args[0], l, c);
			DrawTriangleFan(points.data(), (int)points.size(),
				ValueToColor(args[1], l, c));
			return Value::None();
		});
		define("DrawPoly", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 5)
				throw ArgumentError("DrawPoly(center, sides, radius, rot, color)",
					l, c);
			DrawPoly(ValueToVector2(args[0], l, c), (int)args[1].asInt(),
				(float)args[2].asFloat(), (float)args[3].asFloat(),
				ValueToColor(args[4], l, c));
			return Value::None();
		});
		define("DrawPolyLinesEx", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 6)
				throw ArgumentError(
					"DrawPolyLinesEx(center, sides, radius, rot, thick, color)", l,
					c);
			DrawPolyLinesEx(ValueToVector2(args[0], l, c), (int)args[1].asInt(),
				(float)args[2].asFloat(), (float)args[3].asFloat(),
				(float)args[4].asFloat(), ValueToColor(args[5], l, c));
			return Value::None();
		});
		define("IsKeyPressed", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("IsKeyPressed(key)", l, c);
			return Value::Bool(IsKeyPressed((int)args[0].asInt()));
		});
		define("IsKeyPressedRepeat", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("IsKeyPressedRepeat(key)", l, c);
			return Value::Bool(IsKeyPressedRepeat((int)args[0].asInt()));
		});
		define("IsKeyDown", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("IsKeyDown(key)", l, c);
			return Value::Bool(IsKeyDown((int)args[0].asInt()));
		});
		define("IsKeyReleased", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("IsKeyReleased(key)", l, c);
			return Value::Bool(IsKeyReleased((int)args[0].asInt()));
		});
		define("IsKeyUp", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("IsKeyUp(key)", l, c);
			return Value::Bool(IsKeyUp((int)args[0].asInt()));
		});
		define("GetKeyPressed", [](const vector<Value> &args, int l, int c) {
			return Value::Int(GetKeyPressed());
		});
		define("GetCharPressed", [](const vector<Value> &args, int l, int c) {
			return Value::Int(GetCharPressed());
		});
		define("SetExitKey", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("SetExitKey(key)", l, c);
			SetExitKey((int)args[0].asInt());
			return Value::None();
		});
		define("IsMouseButtonPressed", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("IsMouseButtonPressed(button)", l, c);
			return Value::Bool(IsMouseButtonPressed((int)args[0].asInt()));
		});
		define("IsMouseButtonDown", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("IsMouseButtonDown(button)", l, c);
			return Value::Bool(IsMouseButtonDown((int)args[0].asInt()));
		});
		define("IsMouseButtonReleased", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("IsMouseButtonReleased(button)", l, c);
			return Value::Bool(IsMouseButtonReleased((int)args[0].asInt()));
		});
		define("IsMouseButtonUp", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("IsMouseButtonUp(button)", l, c);
			return Value::Bool(IsMouseButtonUp((int)args[0].asInt()));
		});
		define("GetMouseX", [](const vector<Value> &args, int l, int c) {
			return Value::Int(GetMouseX());
		});
		define("GetMouseY", [](const vector<Value> &args, int l, int c) {
			return Value::Int(GetMouseY());
		});
		define("GetMousePosition", [=](const vector<Value> &args, int l, int c) {
			return Vector2ToValue(GetMousePosition());
		});
		define("GetMouseDelta", [=](const vector<Value> &args, int l, int c) {
			return Vector2ToValue(GetMouseDelta());
		});
		define("SetMousePosition", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("SetMousePosition(x, y)", l, c);
			SetMousePosition((int)args[0].asInt(), (int)args[1].asInt());
			return Value::None();
		});
		define("SetMouseOffset", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("SetMouseOffset(x, y)", l, c);
			SetMouseOffset((int)args[0].asInt(), (int)args[1].asInt());
			return Value::None();
		});
		define("SetMouseScale", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("SetMouseScale(x, y)", l, c);
			SetMouseScale((float)args[0].asFloat(), (float)args[1].asFloat());
			return Value::None();
		});
		define("GetMouseWheelMove", [](const vector<Value> &args, int l, int c) {
			return Value::Float(GetMouseWheelMove());
		});
		define("GetMouseWheelMoveV", [=](const vector<Value> &args, int l, int c) {
			return Vector2ToValue(GetMouseWheelMoveV());
		});
		define("SetMouseCursor", [](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("SetMouseCursor(cursor)", l, c);
			SetMouseCursor((int)args[0].asInt());
			return Value::None();
		});
		define("Image", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5)
				throw ArgumentError(
					"Image(data_ptr, width, height, mipmaps, format)", l, c);
			auto inst = std::make_shared<InstanceObject>(imageClass.get());
			inst->fields["data"] = args[0];
			inst->fields["width"] = args[1];
			inst->fields["height"] = args[2];
			inst->fields["mipmaps"] = args[3];
			inst->fields["format"] = args[4];
			Value v;
			v.type = ValueType::INSTANCE;
			v.ref = inst;
			return v;
		});
		define("Texture2D", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5)
				throw ArgumentError("Texture2D(id, width, height, mipmaps, format)",
					l, c);
			auto inst = std::make_shared<InstanceObject>(textureClass.get());
			inst->fields["id"] = args[0];
			inst->fields["width"] = args[1];
			inst->fields["height"] = args[2];
			inst->fields["mipmaps"] = args[3];
			inst->fields["format"] = args[4];
			Value v;
			v.type = ValueType::INSTANCE;
			v.ref = inst;
			return v;
		});
		define("LoadImage", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("LoadImage(fileName)", l, c);
			Image img = LoadImage(args[0].asString().c_str());
			return ImageToValue(img);
		});
		define("LoadImageColors", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("LoadImageColors(img)", l, c);
			Image img = ValueToImage(args[0], l, c);
			Color *pixels = LoadImageColors(img);
			std::vector<Value> colorPix;
			colorPix.reserve(img.width * img.height);
			for (size_t i = 0; i < img.width * img.height; i++)
				colorPix.push_back(
					MakeColor(pixels[i].r, pixels[i].g, pixels[i].b, pixels[i].a));
			return Value::List(colorPix);
		});
		define("UpdateImagePixels", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("UpdateImagePixels(img, PixelList)", l, c);
			Image img = ValueToImage(args[0], l, c);
			auto *vec = static_cast<ListObject *>(args[1].ref.get());
			Color *pixels = LoadImageColors(img);
			size_t limit;
			if (vec->elements.size() != img.width * img.height)
				throw Warning(
					"list size does not match with the image's pixels. The result "
					"may "
					"not be correct, try resizing the image: ImageResizeNN(img, "
					"New_Width, New_Height)",
					l, c);
			limit = std::min((size_t)img.width * (size_t)img.height,
				vec->elements.size());
			for (size_t i = 0; i < limit; i++)
				pixels[i] = ValueToColor(vec->elements[i], l, c);
			return Value::None();
		});
		define("LoadImageFromScreen", [=](const vector<Value> &args, int l, int c) {
			if (!args.empty())
				throw ArgumentError("LoadImageFromScreen()", l, c);
			Image img = LoadImageFromScreen();
			return ImageToValue(img);
		});
		define("ImageResizeNN", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3)
				throw ArgumentError("ImageResizeNN(image, newWidth, newHeight)", l,
					c);
			Image img = ValueToImage(args[0], l, c);
			ImageResizeNN(&img, (int)args[1].asInt(), (int)args[2].asInt());
			auto inst = static_cast<InstanceObject *>(args[0].ref.get());
			inst->fields["data"] = Value::pInt(img.data);
			inst->fields["width"] = Value::Int(img.width);
			inst->fields["height"] = Value::Int(img.height);
			return Value::None();
		});
		define("ImageColorTint", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("ImageColorTint(image, color)", l, c);
			Image img = ValueToImage(args[0], l, c);
			ImageColorTint(&img, ValueToColor(args[1], l, c));
			auto inst = static_cast<InstanceObject *>(args[0].ref.get());
			inst->fields["data"] = Value::pInt(img.data);
			return Value::None();
		});
		define("ImageDither", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5)
				throw ArgumentError("ImageDither(image, rBpp, gBpp, bBpp, aBpp)", l,
					c);
			Image img = ValueToImage(args[0], l, c);
			ImageDither(&img, (int)args[1].asInt(), (int)args[2].asInt(),
				(int)args[3].asInt(), (int)args[4].asInt());
			auto inst = static_cast<InstanceObject *>(args[0].ref.get());
			inst->fields["data"] = Value::pInt(img.data);
			inst->fields["format"] = Value::Int(img.format);
			inst->fields["width"] = Value::Int(img.width);
			inst->fields["height"] = Value::Int(img.height);
			return Value::None();
		});
		define("ImageApplyPalette", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("ImageApplyPalette(image, colorList)", l, c);
			Image img = ValueToImage(args[0], l, c);
			vector<Color> palette;
			auto list = static_cast<ListObject *>(args[1].ref.get());
			for (const auto &val : list->elements)
				palette.push_back(ValueToColor(val, l, c));
			Color *pixels = LoadImageColors(img);
			for (int i = 0; i < img.width * img.height; i++) {
				Color current = pixels[i];
				Color closest = palette[0];
				int minDistance = INT_MAX;
				for (Color p : palette) {
					int rDiff = current.r - p.r;
					int gDiff = current.g - p.g;
					int bDiff = current.b - p.b;
					double dist = (rDiff * rDiff * 0.30) + (gDiff * gDiff * 0.59) +
									  (bDiff * bDiff * 0.11);
					if (dist < minDistance) {
						minDistance = dist;
						closest = p;
					}
				}
				pixels[i] = closest;
			}
			for (int y = 0; y < img.height; y++)
				for (int x = 0; x < img.width; x++)
					ImageDrawPixel(&img, x, y, pixels[y * img.width + x]);
			UnloadImageColors(pixels);
			return Value::None();
		});
		define("ImageColorGrayscale", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("ImageColorGrayscale(image)", l, c);
			Image img = ValueToImage(args[0], l, c);
			ImageColorGrayscale(&img);
			auto inst = static_cast<InstanceObject *>(args[0].ref.get());
			inst->fields["data"] = Value::pInt(img.data);
			inst->fields["format"] = Value::Int(img.format);
			inst->fields["width"] = Value::Int(img.width);
			inst->fields["height"] = Value::Int(img.height);
			return Value::None();
		});
		define("ImageColorInvert", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("ImageColorInvert(image)", l, c);
			Image img = ValueToImage(args[0], l, c);
			ImageColorInvert(&img);
			auto inst = static_cast<InstanceObject *>(args[0].ref.get());
			inst->fields["data"] = Value::pInt(img.data);
			return Value::None();
		});
		define("ImageColorContrast", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError(
					"ImageColorContrast(image, contrastFloat)", l, c);
			Image img = ValueToImage(args[0], l, c);
			ImageColorContrast(&img, (float)args[1].asFloat());
			auto inst = static_cast<InstanceObject *>(args[0].ref.get());
			inst->fields["data"] = Value::pInt(img.data);
			return Value::None();
		});
		define("ImageColorBrightness", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError(
					"ImageColorBrightness(image, brightnessInt)", l, c);
			Image img = ValueToImage(args[0], l, c);
			ImageColorBrightness(&img, (int)args[1].asInt());
			auto inst = static_cast<InstanceObject *>(args[0].ref.get());
			inst->fields["data"] = Value::pInt(img.data);
			return Value::None();
		});
		define("ImageBlurGaussian", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("ImageBlurGaussian(image, blur)", l, c);
			Image img = ValueToImage(args[0], l, c);
			ImageBlurGaussian(&img, args[1].asInt());
			auto inst = static_cast<InstanceObject *>(args[0].ref.get());
			inst->fields["data"] = Value::pInt(img.data);
			return Value::None();
		});
		define("ImageFlipVertical", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("ImageFlipVertical(image)", l, c);
			Image img = ValueToImage(args[0], l, c);
			ImageFlipVertical(&img);
			auto inst = static_cast<InstanceObject *>(args[0].ref.get());
			inst->fields["data"] = Value::pInt(img.data);
			return Value::None();
		});
		define("ImageFlipHorizontal", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("ImageFlipHorizontal(image)", l, c);
			Image img = ValueToImage(args[0], l, c);
			ImageFlipHorizontal(&img);
			auto inst = static_cast<InstanceObject *>(args[0].ref.get());
			inst->fields["data"] = Value::pInt(img.data);
			return Value::None();
		});
		define("ExportImage", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("ExportImage(image, fileName)", l, c);
			Image img = ValueToImage(args[0], l, c);
			bool success = ExportImage(img, args[1].asString().c_str());
			return Value::Bool(success);
		});
		define("UnloadImage", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("UnloadImage(image)", l, c);
			UnloadImage(ValueToImage(args[0], l, c));
			return Value::None();
		});
		define("LoadTexture", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("LoadTexture(fileName)", l, c);
			Texture2D tex = LoadTexture(args[0].asString().c_str());
			return TextureToValue(tex);
		});
		define("LoadTextureFromImage", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("LoadTextureFromImage(image)", l, c);
			Texture2D tex = LoadTextureFromImage(ValueToImage(args[0], l, c));
			return TextureToValue(tex);
		});
		define("UnloadTexture", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("UnloadTexture(texture)", l, c);
			UnloadTexture(ValueToTexture(args[0], l, c));
			return Value::None();
		});
		define("DrawTexture", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4)
				throw ArgumentError("DrawTexture(texture, posX, posY, tintColor)",
					l, c);
			DrawTexture(ValueToTexture(args[0], l, c), (int)args[1].asInt(),
				(int)args[2].asInt(), ValueToColor(args[3], l, c));
			return Value::None();
		});
		define("DrawTextureV", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3)
				throw ArgumentError("DrawTextureV(texture, positionVec, tintColor)",
					l, c);
			DrawTextureV(ValueToTexture(args[0], l, c),
				ValueToVector2(args[1], l, c),
				ValueToColor(args[2], l, c));
			return Value::None();
		});
		define("DrawTextureEx", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5)
				throw ArgumentError(
					"DrawTextureEx(texture, positionVec, rotation, scale, "
					"tintColor)",
					l, c);
			DrawTextureEx(ValueToTexture(args[0], l, c),
				ValueToVector2(args[1], l, c), (float)args[2].asFloat(),
				(float)args[3].asFloat(), ValueToColor(args[4], l, c));
			return Value::None();
		});
		define("DrawTextureRec", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4)
				throw ArgumentError(
					"DrawTextureRec(texture, sourceRec, positionVec, tintColor)", l,
					c);
			DrawTextureRec(
				ValueToTexture(args[0], l, c), ValueToRect(args[1], l, c),
				ValueToVector2(args[2], l, c), ValueToColor(args[3], l, c));
			return Value::None();
		});
		define("DrawTexturePro", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 6)
				throw ArgumentError("DrawTexturePro(texture, sourceRec, destRec, originVec, rotation, tintColor)", l, c);
			DrawTexturePro(
				ValueToTexture(args[0], l, c),
				ValueToRect(args[1], l, c),
				ValueToRect(args[2], l, c),
				ValueToVector2(args[3], l, c),
				static_cast<float>(args[4].asFloat()),
				ValueToColor(args[5], l, c));
			return Value::None();
		});
		define("InitAudioDevice", [=](const vector<Value> &args, int l, int c) {
			InitAudioDevice();
			return Value::None();
		});
		define("CloseAudioDevice", [=](const vector<Value> &args, int l, int c) {
			CloseAudioDevice();
			return Value::None();
		});
		define("LoadSound", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("LoadSound(fileName)", l, c);
			Sound s = LoadSound(args[0].asString().c_str());
			return SoundToValue(s);
		});
		define("UnloadSound", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("UnloadSound(sound)", l, c);
			UnloadSound(ValueToSound(args[0], l, c));
			return Value::None();
		});
		define("LoadMusicStream", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("LoadMusicStream(fileName)", l, c);
			Music m = LoadMusicStream(args[0].asString().c_str());
			return MusicToValue(m);
		});
		define("UnloadMusicStream", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("UnloadMusicStream(music)", l, c);
			UnloadMusicStream(ValueToMusic(args[0], l, c));
			return Value::None();
		});
		define("SetMasterVolume", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("SetMasterVolume(float)", l, c);
			SetMasterVolume((float)args[0].asFloat());
			return Value::None();
		});
		define("PlaySound", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("PlaySound(sound)", l, c);
			PlaySound(ValueToSound(args[0], l, c));
			return Value::None();
		});
		define("StopSound", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("StopSound(sound)", l, c);
			StopSound(ValueToSound(args[0], l, c));
			return Value::None();
		});
		define("SetSoundPitch", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("SetSoundPitch(sound, pitchFloat)", l, c);
			// 1.0 is normal, 0.5 is slow/deep, 2.0 is fast/high
			SetSoundPitch(ValueToSound(args[0], l, c), (float)args[1].asFloat());
			return Value::None();
		});
		define("SetSoundVolume", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("SetSoundVolume(sound, volumeFloat)", l, c);
			SetSoundVolume(ValueToSound(args[0], l, c), (float)args[1].asFloat());
			return Value::None();
		});
		define("SetSoundPan", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("SetSoundPan(sound, panFloat)", l, c);
			// 0.0 is left speaker, 0.5 is middle, 1.0 is right speaker
			SetSoundPan(ValueToSound(args[0], l, c), (float)args[1].asFloat());
			return Value::None();
		});
		define("PlayMusicStream", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("PlayMusicStream(music)", l, c);
			PlayMusicStream(ValueToMusic(args[0], l, c));
			return Value::None();
		});
		define("UpdateMusicStream", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1)
				throw ArgumentError("UpdateMusicStream(music)", l, c);
			UpdateMusicStream(ValueToMusic(args[0], l, c));
			return Value::None();
		});
		define("SetMusicPitch", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("SetMusicPitch(music, pitchFloat)", l, c);
			SetMusicPitch(ValueToMusic(args[0], l, c), (float)args[1].asFloat());
			return Value::None();
		});
		define("SetMusicVolume", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2)
				throw ArgumentError("SetMusicVolume(music, volumeFloat)", l, c);
			SetMusicVolume(ValueToMusic(args[0], l, c), (float)args[1].asFloat());
			return Value::None();
		});


		// --- AUTO-GENERATED RAYLIB BINDINGS ---
		define("IsWindowReady", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("IsWindowReady expects 0 arguments", l, c);
			return Value::Bool(IsWindowReady());
		});
		define("IsWindowFullscreen", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("IsWindowFullscreen expects 0 arguments", l, c);
			return Value::Bool(IsWindowFullscreen());
		});
		define("IsWindowHidden", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("IsWindowHidden expects 0 arguments", l, c);
			return Value::Bool(IsWindowHidden());
		});
		define("IsWindowMinimized", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("IsWindowMinimized expects 0 arguments", l, c);
			return Value::Bool(IsWindowMinimized());
		});
		define("IsWindowMaximized", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("IsWindowMaximized expects 0 arguments", l, c);
			return Value::Bool(IsWindowMaximized());
		});
		define("IsWindowFocused", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("IsWindowFocused expects 0 arguments", l, c);
			return Value::Bool(IsWindowFocused());
		});
		define("IsWindowResized", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("IsWindowResized expects 0 arguments", l, c);
			return Value::Bool(IsWindowResized());
		});
		define("IsWindowState", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("IsWindowState expects 1 arguments", l, c);
			return Value::Bool(IsWindowState((unsigned int)args[0].asInt()));
		});
		define("SetWindowState", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("SetWindowState expects 1 arguments", l, c);
			SetWindowState((unsigned int)args[0].asInt());
			return Value::None();
		});
		define("ClearWindowState", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("ClearWindowState expects 1 arguments", l, c);
			ClearWindowState((unsigned int)args[0].asInt());
			return Value::None();
		});
		define("ToggleBorderlessWindowed", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("ToggleBorderlessWindowed expects 0 arguments", l, c);
			ToggleBorderlessWindowed();
			return Value::None();
		});
		define("MaximizeWindow", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("MaximizeWindow expects 0 arguments", l, c);
			MaximizeWindow();
			return Value::None();
		});
		define("MinimizeWindow", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("MinimizeWindow expects 0 arguments", l, c);
			MinimizeWindow();
			return Value::None();
		});
		define("RestoreWindow", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("RestoreWindow expects 0 arguments", l, c);
			RestoreWindow();
			return Value::None();
		});
		define("SetWindowIcon", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("SetWindowIcon expects 1 arguments", l, c);
			SetWindowIcon(ValueToImage(args[0], l, c));
			return Value::None();
		});
		define("SetWindowMonitor", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("SetWindowMonitor expects 1 arguments", l, c);
			SetWindowMonitor((int)args[0].asInt());
			return Value::None();
		});
		define("SetWindowMinSize", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("SetWindowMinSize expects 2 arguments", l, c);
			SetWindowMinSize((int)args[0].asInt(), (int)args[1].asInt());
			return Value::None();
		});
		define("SetWindowMaxSize", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("SetWindowMaxSize expects 2 arguments", l, c);
			SetWindowMaxSize((int)args[0].asInt(), (int)args[1].asInt());
			return Value::None();
		});
		define("SetWindowSize", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("SetWindowSize expects 2 arguments", l, c);
			SetWindowSize((int)args[0].asInt(), (int)args[1].asInt());
			return Value::None();
		});
		define("SetWindowOpacity", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("SetWindowOpacity expects 1 arguments", l, c);
			SetWindowOpacity((float)args[0].asFloat());
			return Value::None();
		});
		define("SetWindowFocused", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("SetWindowFocused expects 0 arguments", l, c);
			SetWindowFocused();
			return Value::None();
		});
		define("GetScreenWidth", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetScreenWidth expects 0 arguments", l, c);
			return Value::Int(GetScreenWidth());
		});
		define("GetScreenHeight", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetScreenHeight expects 0 arguments", l, c);
			return Value::Int(GetScreenHeight());
		});
		define("GetRenderWidth", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetRenderWidth expects 0 arguments", l, c);
			return Value::Int(GetRenderWidth());
		});
		define("GetRenderHeight", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetRenderHeight expects 0 arguments", l, c);
			return Value::Int(GetRenderHeight());
		});
		define("GetMonitorCount", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetMonitorCount expects 0 arguments", l, c);
			return Value::Int(GetMonitorCount());
		});
		define("GetCurrentMonitor", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetCurrentMonitor expects 0 arguments", l, c);
			return Value::Int(GetCurrentMonitor());
		});
		define("GetMonitorPosition", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("GetMonitorPosition expects 1 arguments", l, c);
			return Vector2ToValue(GetMonitorPosition((int)args[0].asInt()));
		});
		define("GetMonitorPhysicalWidth", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("GetMonitorPhysicalWidth expects 1 arguments", l, c);
			return Value::Int(GetMonitorPhysicalWidth((int)args[0].asInt()));
		});
		define("GetMonitorPhysicalHeight", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("GetMonitorPhysicalHeight expects 1 arguments", l, c);
			return Value::Int(GetMonitorPhysicalHeight((int)args[0].asInt()));
		});
		define("GetMonitorRefreshRate", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("GetMonitorRefreshRate expects 1 arguments", l, c);
			return Value::Int(GetMonitorRefreshRate((int)args[0].asInt()));
		});
		define("GetWindowPosition", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetWindowPosition expects 0 arguments", l, c);
			return Vector2ToValue(GetWindowPosition());
		});
		define("GetWindowScaleDPI", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetWindowScaleDPI expects 0 arguments", l, c);
			return Vector2ToValue(GetWindowScaleDPI());
		});
		define("GetClipboardImage", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetClipboardImage expects 0 arguments", l, c);
			return ImageToValue(GetClipboardImage());
		});
		define("EnableEventWaiting", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("EnableEventWaiting expects 0 arguments", l, c);
			EnableEventWaiting();
			return Value::None();
		});
		define("DisableEventWaiting", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("DisableEventWaiting expects 0 arguments", l, c);
			DisableEventWaiting();
			return Value::None();
		});
		define("ShowCursor", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("ShowCursor expects 0 arguments", l, c);
			ShowCursor();
			return Value::None();
		});
		define("HideCursor", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("HideCursor expects 0 arguments", l, c);
			HideCursor();
			return Value::None();
		});
		define("IsCursorHidden", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("IsCursorHidden expects 0 arguments", l, c);
			return Value::Bool(IsCursorHidden());
		});
		define("EnableCursor", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("EnableCursor expects 0 arguments", l, c);
			EnableCursor();
			return Value::None();
		});
		define("DisableCursor", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("DisableCursor expects 0 arguments", l, c);
			DisableCursor();
			return Value::None();
		});
		define("IsCursorOnScreen", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("IsCursorOnScreen expects 0 arguments", l, c);
			return Value::Bool(IsCursorOnScreen());
		});
		define("EndMode2D", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("EndMode2D expects 0 arguments", l, c);
			EndMode2D();
			return Value::None();
		});
		define("BeginScissorMode", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("BeginScissorMode expects 4 arguments", l, c);
			BeginScissorMode((int)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt(), (int)args[3].asInt());
			return Value::None();
		});
		define("EndScissorMode", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("EndScissorMode expects 0 arguments", l, c);
			EndScissorMode();
			return Value::None();
		});
		define("EndVrStereoMode", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("EndVrStereoMode expects 0 arguments", l, c);
			EndVrStereoMode();
			return Value::None();
		});
		define("GetTime", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetTime expects 0 arguments", l, c);
			return Value::Float(GetTime());
		});
		define("GetFPS", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetFPS expects 0 arguments", l, c);
			return Value::Int(GetFPS());
		});
		define("SwapScreenBuffer", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("SwapScreenBuffer expects 0 arguments", l, c);
			SwapScreenBuffer();
			return Value::None();
		});
		define("PollInputEvents", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("PollInputEvents expects 0 arguments", l, c);
			PollInputEvents();
			return Value::None();
		});
		define("WaitTime", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("WaitTime expects 1 arguments", l, c);
			WaitTime((double)args[0].asFloat());
			return Value::None();
		});
		define("SetRandomSeed", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("SetRandomSeed expects 1 arguments", l, c);
			SetRandomSeed((unsigned int)args[0].asInt());
			return Value::None();
		});
		define("GetRandomValue", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("GetRandomValue expects 2 arguments", l, c);
			return Value::Int(GetRandomValue((int)args[0].asInt(), (int)args[1].asInt()));
		});
		define("SetTraceLogLevel", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("SetTraceLogLevel expects 1 arguments", l, c);
			SetTraceLogLevel((int)args[0].asInt());
			return Value::None();
		});
		define("IsFileDropped", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("IsFileDropped expects 0 arguments", l, c);
			return Value::Bool(IsFileDropped());
		});
		define("SetAutomationEventBaseFrame", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("SetAutomationEventBaseFrame expects 1 arguments", l, c);
			SetAutomationEventBaseFrame((int)args[0].asInt());
			return Value::None();
		});
		define("StartAutomationEventRecording", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("StartAutomationEventRecording expects 0 arguments", l, c);
			StartAutomationEventRecording();
			return Value::None();
		});
		define("StopAutomationEventRecording", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("StopAutomationEventRecording expects 0 arguments", l, c);
			StopAutomationEventRecording();
			return Value::None();
		});
		define("IsGamepadAvailable", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("IsGamepadAvailable expects 1 arguments", l, c);
			return Value::Bool(IsGamepadAvailable((int)args[0].asInt()));
		});
		define("IsGamepadButtonPressed", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("IsGamepadButtonPressed expects 2 arguments", l, c);
			return Value::Bool(IsGamepadButtonPressed((int)args[0].asInt(), (int)args[1].asInt()));
		});
		define("IsGamepadButtonDown", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("IsGamepadButtonDown expects 2 arguments", l, c);
			return Value::Bool(IsGamepadButtonDown((int)args[0].asInt(), (int)args[1].asInt()));
		});
		define("IsGamepadButtonReleased", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("IsGamepadButtonReleased expects 2 arguments", l, c);
			return Value::Bool(IsGamepadButtonReleased((int)args[0].asInt(), (int)args[1].asInt()));
		});
		define("IsGamepadButtonUp", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("IsGamepadButtonUp expects 2 arguments", l, c);
			return Value::Bool(IsGamepadButtonUp((int)args[0].asInt(), (int)args[1].asInt()));
		});
		define("GetGamepadButtonPressed", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetGamepadButtonPressed expects 0 arguments", l, c);
			return Value::Int(GetGamepadButtonPressed());
		});
		define("GetGamepadAxisCount", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("GetGamepadAxisCount expects 1 arguments", l, c);
			return Value::Int(GetGamepadAxisCount((int)args[0].asInt()));
		});
		define("GetGamepadAxisMovement", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("GetGamepadAxisMovement expects 2 arguments", l, c);
			return Value::Float(GetGamepadAxisMovement((int)args[0].asInt(), (int)args[1].asInt()));
		});
		define("SetGamepadVibration", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("SetGamepadVibration expects 4 arguments", l, c);
			SetGamepadVibration((int)args[0].asInt(), (float)args[1].asFloat(), (float)args[2].asFloat(), (float)args[3].asFloat());
			return Value::None();
		});
		define("GetTouchX", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetTouchX expects 0 arguments", l, c);
			return Value::Int(GetTouchX());
		});
		define("GetTouchY", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetTouchY expects 0 arguments", l, c);
			return Value::Int(GetTouchY());
		});
		define("GetTouchPosition", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("GetTouchPosition expects 1 arguments", l, c);
			return Vector2ToValue(GetTouchPosition((int)args[0].asInt()));
		});
		define("GetTouchPointId", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("GetTouchPointId expects 1 arguments", l, c);
			return Value::Int(GetTouchPointId((int)args[0].asInt()));
		});
		define("GetTouchPointCount", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetTouchPointCount expects 0 arguments", l, c);
			return Value::Int(GetTouchPointCount());
		});
		define("SetGesturesEnabled", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("SetGesturesEnabled expects 1 arguments", l, c);
			SetGesturesEnabled((unsigned int)args[0].asInt());
			return Value::None();
		});
		define("IsGestureDetected", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("IsGestureDetected expects 1 arguments", l, c);
			return Value::Bool(IsGestureDetected((unsigned int)args[0].asInt()));
		});
		define("GetGestureDetected", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetGestureDetected expects 0 arguments", l, c);
			return Value::Int(GetGestureDetected());
		});
		define("GetGestureHoldDuration", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetGestureHoldDuration expects 0 arguments", l, c);
			return Value::Float(GetGestureHoldDuration());
		});
		define("GetGestureDragVector", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetGestureDragVector expects 0 arguments", l, c);
			return Vector2ToValue(GetGestureDragVector());
		});
		define("GetGestureDragAngle", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetGestureDragAngle expects 0 arguments", l, c);
			return Value::Float(GetGestureDragAngle());
		});
		define("GetGesturePinchVector", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetGesturePinchVector expects 0 arguments", l, c);
			return Vector2ToValue(GetGesturePinchVector());
		});
		define("GetGesturePinchAngle", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetGesturePinchAngle expects 0 arguments", l, c);
			return Value::Float(GetGesturePinchAngle());
		});
		define("SetShapesTexture", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("SetShapesTexture expects 2 arguments", l, c);
			SetShapesTexture(ValueToTexture(args[0], l, c), ValueToRect(args[1], l, c));
			return Value::None();
		});
		define("GetShapesTexture", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetShapesTexture expects 0 arguments", l, c);
			return TextureToValue(GetShapesTexture());
		});
		define("DrawLineDashed", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("DrawLineDashed expects 5 arguments", l, c);
			DrawLineDashed(ValueToVector2(args[0], l, c), ValueToVector2(args[1], l, c), (int)args[2].asInt(), (int)args[3].asInt(), ValueToColor(args[4], l, c));
			return Value::None();
		});
		define("DrawCircleSectorLines", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 6) throw ArgumentError("DrawCircleSectorLines expects 6 arguments", l, c);
			DrawCircleSectorLines(ValueToVector2(args[0], l, c), (float)args[1].asFloat(), (float)args[2].asFloat(), (float)args[3].asFloat(), (int)args[4].asInt(), ValueToColor(args[5], l, c));
			return Value::None();
		});
		define("DrawCircleLinesV", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("DrawCircleLinesV expects 3 arguments", l, c);
			DrawCircleLinesV(ValueToVector2(args[0], l, c), (float)args[1].asFloat(), ValueToColor(args[2], l, c));
			return Value::None();
		});
		define("DrawEllipseV", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("DrawEllipseV expects 4 arguments", l, c);
			DrawEllipseV(ValueToVector2(args[0], l, c), (float)args[1].asFloat(), (float)args[2].asFloat(), ValueToColor(args[3], l, c));
			return Value::None();
		});
		define("DrawEllipseLines", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("DrawEllipseLines expects 5 arguments", l, c);
			DrawEllipseLines((int)args[0].asInt(), (int)args[1].asInt(), (float)args[2].asFloat(), (float)args[3].asFloat(), ValueToColor(args[4], l, c));
			return Value::None();
		});
		define("DrawEllipseLinesV", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("DrawEllipseLinesV expects 4 arguments", l, c);
			DrawEllipseLinesV(ValueToVector2(args[0], l, c), (float)args[1].asFloat(), (float)args[2].asFloat(), ValueToColor(args[3], l, c));
			return Value::None();
		});
		define("DrawRingLines", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 7) throw ArgumentError("DrawRingLines expects 7 arguments", l, c);
			DrawRingLines(ValueToVector2(args[0], l, c), (float)args[1].asFloat(), (float)args[2].asFloat(), (float)args[3].asFloat(), (float)args[4].asFloat(), (int)args[5].asInt(), ValueToColor(args[6], l, c));
			return Value::None();
		});
		define("DrawRectangleGradientH", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 6) throw ArgumentError("DrawRectangleGradientH expects 6 arguments", l, c);
			DrawRectangleGradientH((int)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt(), (int)args[3].asInt(), ValueToColor(args[4], l, c), ValueToColor(args[5], l, c));
			return Value::None();
		});
		define("DrawRectangleGradientEx", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("DrawRectangleGradientEx expects 5 arguments", l, c);
			DrawRectangleGradientEx(ValueToRect(args[0], l, c), ValueToColor(args[1], l, c), ValueToColor(args[2], l, c), ValueToColor(args[3], l, c), ValueToColor(args[4], l, c));
			return Value::None();
		});
		define("DrawRectangleLines", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("DrawRectangleLines expects 5 arguments", l, c);
			DrawRectangleLines((int)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt(), (int)args[3].asInt(), ValueToColor(args[4], l, c));
			return Value::None();
		});
		define("DrawRectangleLinesEx", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("DrawRectangleLinesEx expects 3 arguments", l, c);
			DrawRectangleLinesEx(ValueToRect(args[0], l, c), (float)args[1].asFloat(), ValueToColor(args[2], l, c));
			return Value::None();
		});
		define("DrawRectangleRoundedLines", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("DrawRectangleRoundedLines expects 4 arguments", l, c);
			DrawRectangleRoundedLines(ValueToRect(args[0], l, c), (float)args[1].asFloat(), (int)args[2].asInt(), ValueToColor(args[3], l, c));
			return Value::None();
		});
		define("DrawRectangleRoundedLinesEx", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("DrawRectangleRoundedLinesEx expects 5 arguments", l, c);
			DrawRectangleRoundedLinesEx(ValueToRect(args[0], l, c), (float)args[1].asFloat(), (int)args[2].asInt(), (float)args[3].asFloat(), ValueToColor(args[4], l, c));
			return Value::None();
		});
		define("DrawTriangleLines", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("DrawTriangleLines expects 4 arguments", l, c);
			DrawTriangleLines(ValueToVector2(args[0], l, c), ValueToVector2(args[1], l, c), ValueToVector2(args[2], l, c), ValueToColor(args[3], l, c));
			return Value::None();
		});
		define("DrawPolyLines", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("DrawPolyLines expects 5 arguments", l, c);
			DrawPolyLines(ValueToVector2(args[0], l, c), (int)args[1].asInt(), (float)args[2].asFloat(), (float)args[3].asFloat(), ValueToColor(args[4], l, c));
			return Value::None();
		});
		define("DrawSplineSegmentLinear", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("DrawSplineSegmentLinear expects 4 arguments", l, c);
			DrawSplineSegmentLinear(ValueToVector2(args[0], l, c), ValueToVector2(args[1], l, c), (float)args[2].asFloat(), ValueToColor(args[3], l, c));
			return Value::None();
		});
		define("DrawSplineSegmentBasis", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 6) throw ArgumentError("DrawSplineSegmentBasis expects 6 arguments", l, c);
			DrawSplineSegmentBasis(ValueToVector2(args[0], l, c), ValueToVector2(args[1], l, c), ValueToVector2(args[2], l, c), ValueToVector2(args[3], l, c), (float)args[4].asFloat(), ValueToColor(args[5], l, c));
			return Value::None();
		});
		define("DrawSplineSegmentCatmullRom", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 6) throw ArgumentError("DrawSplineSegmentCatmullRom expects 6 arguments", l, c);
			DrawSplineSegmentCatmullRom(ValueToVector2(args[0], l, c), ValueToVector2(args[1], l, c), ValueToVector2(args[2], l, c), ValueToVector2(args[3], l, c), (float)args[4].asFloat(), ValueToColor(args[5], l, c));
			return Value::None();
		});
		define("DrawSplineSegmentBezierQuadratic", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("DrawSplineSegmentBezierQuadratic expects 5 arguments", l, c);
			DrawSplineSegmentBezierQuadratic(ValueToVector2(args[0], l, c), ValueToVector2(args[1], l, c), ValueToVector2(args[2], l, c), (float)args[3].asFloat(), ValueToColor(args[4], l, c));
			return Value::None();
		});
		define("DrawSplineSegmentBezierCubic", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 6) throw ArgumentError("DrawSplineSegmentBezierCubic expects 6 arguments", l, c);
			DrawSplineSegmentBezierCubic(ValueToVector2(args[0], l, c), ValueToVector2(args[1], l, c), ValueToVector2(args[2], l, c), ValueToVector2(args[3], l, c), (float)args[4].asFloat(), ValueToColor(args[5], l, c));
			return Value::None();
		});
		define("GetSplinePointLinear", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("GetSplinePointLinear expects 3 arguments", l, c);
			return Vector2ToValue(GetSplinePointLinear(ValueToVector2(args[0], l, c), ValueToVector2(args[1], l, c), (float)args[2].asFloat()));
		});
		define("GetSplinePointBasis", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("GetSplinePointBasis expects 5 arguments", l, c);
			return Vector2ToValue(GetSplinePointBasis(ValueToVector2(args[0], l, c), ValueToVector2(args[1], l, c), ValueToVector2(args[2], l, c), ValueToVector2(args[3], l, c), (float)args[4].asFloat()));
		});
		define("GetSplinePointCatmullRom", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("GetSplinePointCatmullRom expects 5 arguments", l, c);
			return Vector2ToValue(GetSplinePointCatmullRom(ValueToVector2(args[0], l, c), ValueToVector2(args[1], l, c), ValueToVector2(args[2], l, c), ValueToVector2(args[3], l, c), (float)args[4].asFloat()));
		});
		define("GetSplinePointBezierQuad", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("GetSplinePointBezierQuad expects 4 arguments", l, c);
			return Vector2ToValue(GetSplinePointBezierQuad(ValueToVector2(args[0], l, c), ValueToVector2(args[1], l, c), ValueToVector2(args[2], l, c), (float)args[3].asFloat()));
		});
		define("GetSplinePointBezierCubic", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("GetSplinePointBezierCubic expects 5 arguments", l, c);
			return Vector2ToValue(GetSplinePointBezierCubic(ValueToVector2(args[0], l, c), ValueToVector2(args[1], l, c), ValueToVector2(args[2], l, c), ValueToVector2(args[3], l, c), (float)args[4].asFloat()));
		});
		define("CheckCollisionRecs", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("CheckCollisionRecs expects 2 arguments", l, c);
			return Value::Bool(CheckCollisionRecs(ValueToRect(args[0], l, c), ValueToRect(args[1], l, c)));
		});
		define("CheckCollisionCircles", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("CheckCollisionCircles expects 4 arguments", l, c);
			return Value::Bool(CheckCollisionCircles(ValueToVector2(args[0], l, c), (float)args[1].asFloat(), ValueToVector2(args[2], l, c), (float)args[3].asFloat()));
		});
		define("CheckCollisionCircleRec", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("CheckCollisionCircleRec expects 3 arguments", l, c);
			return Value::Bool(CheckCollisionCircleRec(ValueToVector2(args[0], l, c), (float)args[1].asFloat(), ValueToRect(args[2], l, c)));
		});
		define("CheckCollisionCircleLine", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("CheckCollisionCircleLine expects 4 arguments", l, c);
			return Value::Bool(CheckCollisionCircleLine(ValueToVector2(args[0], l, c), (float)args[1].asFloat(), ValueToVector2(args[2], l, c), ValueToVector2(args[3], l, c)));
		});
		define("CheckCollisionPointRec", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("CheckCollisionPointRec expects 2 arguments", l, c);
			return Value::Bool(CheckCollisionPointRec(ValueToVector2(args[0], l, c), ValueToRect(args[1], l, c)));
		});
		define("CheckCollisionPointCircle", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("CheckCollisionPointCircle expects 3 arguments", l, c);
			return Value::Bool(CheckCollisionPointCircle(ValueToVector2(args[0], l, c), ValueToVector2(args[1], l, c), (float)args[2].asFloat()));
		});
		define("CheckCollisionPointTriangle", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("CheckCollisionPointTriangle expects 4 arguments", l, c);
			return Value::Bool(CheckCollisionPointTriangle(ValueToVector2(args[0], l, c), ValueToVector2(args[1], l, c), ValueToVector2(args[2], l, c), ValueToVector2(args[3], l, c)));
		});
		define("CheckCollisionPointLine", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 4) throw ArgumentError("CheckCollisionPointLine expects 4 arguments", l, c);
			return Value::Bool(CheckCollisionPointLine(ValueToVector2(args[0], l, c), ValueToVector2(args[1], l, c), ValueToVector2(args[2], l, c), (int)args[3].asInt()));
		});
		define("LoadImageFromTexture", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("LoadImageFromTexture expects 1 arguments", l, c);
			return ImageToValue(LoadImageFromTexture(ValueToTexture(args[0], l, c)));
		});
		define("IsImageValid", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("IsImageValid expects 1 arguments", l, c);
			return Value::Bool(IsImageValid(ValueToImage(args[0], l, c)));
		});
		define("GenImageColor", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("GenImageColor expects 3 arguments", l, c);
			return ImageToValue(GenImageColor((int)args[0].asInt(), (int)args[1].asInt(), ValueToColor(args[2], l, c)));
		});
		define("GenImageGradientLinear", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("GenImageGradientLinear expects 5 arguments", l, c);
			return ImageToValue(GenImageGradientLinear((int)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt(), ValueToColor(args[3], l, c), ValueToColor(args[4], l, c)));
		});
		define("GenImageGradientRadial", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("GenImageGradientRadial expects 5 arguments", l, c);
			return ImageToValue(GenImageGradientRadial((int)args[0].asInt(), (int)args[1].asInt(), (float)args[2].asFloat(), ValueToColor(args[3], l, c), ValueToColor(args[4], l, c)));
		});
		define("GenImageGradientSquare", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("GenImageGradientSquare expects 5 arguments", l, c);
			return ImageToValue(GenImageGradientSquare((int)args[0].asInt(), (int)args[1].asInt(), (float)args[2].asFloat(), ValueToColor(args[3], l, c), ValueToColor(args[4], l, c)));
		});
		define("GenImageChecked", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 6) throw ArgumentError("GenImageChecked expects 6 arguments", l, c);
			return ImageToValue(GenImageChecked((int)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt(), (int)args[3].asInt(), ValueToColor(args[4], l, c), ValueToColor(args[5], l, c)));
		});
		define("GenImageWhiteNoise", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("GenImageWhiteNoise expects 3 arguments", l, c);
			return ImageToValue(GenImageWhiteNoise((int)args[0].asInt(), (int)args[1].asInt(), (float)args[2].asFloat()));
		});
		define("GenImagePerlinNoise", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("GenImagePerlinNoise expects 5 arguments", l, c);
			return ImageToValue(GenImagePerlinNoise((int)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt(), (int)args[3].asInt(), (float)args[4].asFloat()));
		});
		define("GenImageCellular", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("GenImageCellular expects 3 arguments", l, c);
			return ImageToValue(GenImageCellular((int)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt()));
		});
		define("ImageCopy", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("ImageCopy expects 1 arguments", l, c);
			return ImageToValue(ImageCopy(ValueToImage(args[0], l, c)));
		});
		define("ImageFromImage", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("ImageFromImage expects 2 arguments", l, c);
			return ImageToValue(ImageFromImage(ValueToImage(args[0], l, c), ValueToRect(args[1], l, c)));
		});
		define("ImageFromChannel", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("ImageFromChannel expects 2 arguments", l, c);
			return ImageToValue(ImageFromChannel(ValueToImage(args[0], l, c), (int)args[1].asInt()));
		});
		define("IsTextureValid", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("IsTextureValid expects 1 arguments", l, c);
			return Value::Bool(IsTextureValid(ValueToTexture(args[0], l, c)));
		});
		define("IsRenderTextureValid", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("IsRenderTextureValid expects 1 arguments", l, c);
			return Value::Bool(IsRenderTextureValid(ValueToRenderTexture(args[0], l, c)));
		});
		define("SetTextureWrap", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("SetTextureWrap expects 2 arguments", l, c);
			SetTextureWrap(ValueToTexture(args[0], l, c), (int)args[1].asInt());
			return Value::None();
		});
		define("ColorIsEqual", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("ColorIsEqual expects 2 arguments", l, c);
			return Value::Bool(ColorIsEqual(ValueToColor(args[0], l, c), ValueToColor(args[1], l, c)));
		});
		define("ColorToInt", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("ColorToInt expects 1 arguments", l, c);
			return Value::Int(ColorToInt(ValueToColor(args[0], l, c)));
		});
		define("GetPixelDataSize", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("GetPixelDataSize expects 3 arguments", l, c);
			return Value::Int(GetPixelDataSize((int)args[0].asInt(), (int)args[1].asInt(), (int)args[2].asInt()));
		});
		define("LoadFontFromImage", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 3) throw ArgumentError("LoadFontFromImage expects 3 arguments", l, c);
			return FontToValue(LoadFontFromImage(ValueToImage(args[0], l, c), ValueToColor(args[1], l, c), (int)args[2].asInt()));
		});
		define("IsFontValid", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("IsFontValid expects 1 arguments", l, c);
			return Value::Bool(IsFontValid(ValueToFont(args[0], l, c)));
		});
		define("DrawTextCodepoint", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 5) throw ArgumentError("DrawTextCodepoint expects 5 arguments", l, c);
			DrawTextCodepoint(ValueToFont(args[0], l, c), (int)args[1].asInt(), ValueToVector2(args[2], l, c), (float)args[3].asFloat(), ValueToColor(args[4], l, c));
			return Value::None();
		});
		define("SetTextLineSpacing", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("SetTextLineSpacing expects 1 arguments", l, c);
			SetTextLineSpacing((int)args[0].asInt());
			return Value::None();
		});
		define("GetGlyphIndex", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("GetGlyphIndex expects 2 arguments", l, c);
			return Value::Int(GetGlyphIndex(ValueToFont(args[0], l, c), (int)args[1].asInt()));
		});
		define("DrawGrid", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("DrawGrid expects 2 arguments", l, c);
			DrawGrid((int)args[0].asInt(), (float)args[1].asFloat());
			return Value::None();
		});
		define("IsAudioDeviceReady", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("IsAudioDeviceReady expects 0 arguments", l, c);
			return Value::Bool(IsAudioDeviceReady());
		});
		define("GetMasterVolume", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 0) throw ArgumentError("GetMasterVolume expects 0 arguments", l, c);
			return Value::Float(GetMasterVolume());
		});
		define("LoadSoundAlias", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("LoadSoundAlias expects 1 arguments", l, c);
			return SoundToValue(LoadSoundAlias(ValueToSound(args[0], l, c)));
		});
		define("IsSoundValid", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("IsSoundValid expects 1 arguments", l, c);
			return Value::Bool(IsSoundValid(ValueToSound(args[0], l, c)));
		});
		define("UnloadSoundAlias", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("UnloadSoundAlias expects 1 arguments", l, c);
			UnloadSoundAlias(ValueToSound(args[0], l, c));
			return Value::None();
		});
		define("PauseSound", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("PauseSound expects 1 arguments", l, c);
			PauseSound(ValueToSound(args[0], l, c));
			return Value::None();
		});
		define("ResumeSound", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("ResumeSound expects 1 arguments", l, c);
			ResumeSound(ValueToSound(args[0], l, c));
			return Value::None();
		});
		define("IsSoundPlaying", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("IsSoundPlaying expects 1 arguments", l, c);
			return Value::Bool(IsSoundPlaying(ValueToSound(args[0], l, c)));
		});
		define("IsMusicValid", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("IsMusicValid expects 1 arguments", l, c);
			return Value::Bool(IsMusicValid(ValueToMusic(args[0], l, c)));
		});
		define("IsMusicStreamPlaying", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("IsMusicStreamPlaying expects 1 arguments", l, c);
			return Value::Bool(IsMusicStreamPlaying(ValueToMusic(args[0], l, c)));
		});
		define("StopMusicStream", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("StopMusicStream expects 1 arguments", l, c);
			StopMusicStream(ValueToMusic(args[0], l, c));
			return Value::None();
		});
		define("PauseMusicStream", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("PauseMusicStream expects 1 arguments", l, c);
			PauseMusicStream(ValueToMusic(args[0], l, c));
			return Value::None();
		});
		define("ResumeMusicStream", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("ResumeMusicStream expects 1 arguments", l, c);
			ResumeMusicStream(ValueToMusic(args[0], l, c));
			return Value::None();
		});
		define("SeekMusicStream", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("SeekMusicStream expects 2 arguments", l, c);
			SeekMusicStream(ValueToMusic(args[0], l, c), (float)args[1].asFloat());
			return Value::None();
		});
		define("SetMusicPan", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 2) throw ArgumentError("SetMusicPan expects 2 arguments", l, c);
			SetMusicPan(ValueToMusic(args[0], l, c), (float)args[1].asFloat());
			return Value::None();
		});
		define("GetMusicTimeLength", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("GetMusicTimeLength expects 1 arguments", l, c);
			return Value::Float(GetMusicTimeLength(ValueToMusic(args[0], l, c)));
		});
		define("GetMusicTimePlayed", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("GetMusicTimePlayed expects 1 arguments", l, c);
			return Value::Float(GetMusicTimePlayed(ValueToMusic(args[0], l, c)));
		});
		define("SetAudioStreamBufferSizeDefault", [=](const vector<Value> &args, int l, int c) {
			if (args.size() != 1) throw ArgumentError("SetAudioStreamBufferSizeDefault expects 1 arguments", l, c);
			SetAudioStreamBufferSizeDefault((int)args[0].asInt());
			return Value::None();
		});

		Value modVal;
		modVal.type = ValueType::CLASS;
		modVal.ref = moduleNamespace;
		env->set("Raylib", modVal, false, false);
	}
