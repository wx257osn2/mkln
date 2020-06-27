#include<filesystem>
#include<iostream>
#include<string_view>
#include<optional>
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include<Windows.h>
#include<shellapi.h>

static constexpr std::string_view help_message(){
	std::string_view message = R"message(
mkln [options] src dest
  options:
    -s: create symbolic link
  src: src file or directory path
  dst: dst file or directory path
)message";
	message.remove_prefix(1);
	message.remove_suffix(1);
	return message;
}

static inline int help(){
	constexpr auto message = help_message();
	std::cerr << message << std::endl;
	return EXIT_FAILURE;
}

struct parameter{
	std::filesystem::path src;
	std::filesystem::path dst;
	bool symbolic;
};

static std::optional<parameter> parse(int argc, char** argv){
	parameter param = {};
	for(int i = 1; i < argc - 2; ++i){
		const std::string_view arg = argv[i];
		if(arg == "-s")
			param.symbolic = true;
		else
			return std::nullopt;
	}
	param.src = argv[argc-2];
	param.dst = argv[argc-1];
	return param;
}

int main(int argc, char** argv){
	if(argc < 3){
		std::cerr << "Too few arguments." << std::endl;
		return help();
	}
	const auto param = parse(argc, argv);
	if(!param){
		std::cerr << "Invalid arguments." << std::endl;
		return help();
	}
	if(!std::filesystem::exists(param->src)){
		std::cerr << "src doesn't exist." << std::endl;
		return help();
	}
	const bool is_directory = std::filesystem::is_directory(param->src);
	if(is_directory && !param->symbolic){
		std::cerr << "Hard link for symbolic link is not supported." << std::endl;
		return EXIT_FAILURE;
	}
	const auto cd = std::filesystem::current_path().string();
	const std::string command = R"cmd(/v:on /c cd /d ")cmd" + cd + R"cmd(" & mklink )cmd" + (is_directory ? "/d \"" : param->symbolic ? "/h \"" :"\"") + param->dst.string() + R"cmd(" ")cmd" + param->src.string() + R"cmd(" & if !errorlevel! neq 0 (pause))cmd";

	::SHELLEXECUTEINFOA einfo = {
		sizeof(::SHELLEXECUTEINFOA),
		0u,
		nullptr,
		"runas",
		"cmd.exe",
		command.c_str(),
		cd.c_str(),
		SW_NORMAL,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		0u,
		nullptr,
		nullptr
	};
	if(::ShellExecuteExA(&einfo) == FALSE){
		LPVOID _msg = nullptr;
		::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, ::GetLastError(), 0u, (LPSTR)&_msg, 0, nullptr);
		std::unique_ptr<std::remove_pointer_t<LPVOID>, decltype(&::LocalFree)> msg{std::move(_msg), &::LocalFree};
		std::cerr << static_cast<LPCSTR>(msg.get()) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
