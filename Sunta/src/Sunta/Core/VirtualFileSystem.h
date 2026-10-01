namespace Sunta
{

class VirtualFileSystem
{
public:
	static void Init();

	static void Mount(const std::string& virtualPrefix, const std::filesystem::path& physicalPath);
	static std::string Resolve(const std::string& virtualPath);

private:
	static std::unordered_map<std::string, std::filesystem::path>& GetMounts();
};

}