
class OLUtils extends Object
	native;

native static function bool IsPS4();
native static function bool IsDingo();
native static function bool IsConsole();
native static function bool IsDLCInstalled();
native static function bool IsPlayingDLC();
native static function bool IsBindableKey(name ButtonName);
native static function OLPlayerController GetOLPC();
native static function GetModMaps(string SubDir, out array<string> MapNames);
native static function GetModPackages(string SubDir, out array<string> PackageNames);
native static function bool LoadModPackage(string PackageName);
native static function bool RegisterModMap(string MapName);
native static function Object LoadObjectFromModPackage(string PackageName, string ObjectName, class ObjectClass);
