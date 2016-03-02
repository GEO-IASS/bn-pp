#include <iostream>
#include <string>
#include <unordered_map>
using namespace std;

void
usage(const char *filename);

void
read_options(unordered_map<string,bool> &options,  int argc, char *argv[]);

int
main(int argc, char *argv[])
{
	char *filename = argv[0];
	if (argc < 2) {
		usage(filename);
		exit(1);
	}

	unordered_map<string,bool> options;
	read_options(options, argc, argv);
	if (options["help"]) {
		usage(filename);
		return 0;
	}

	return 0;
}

void
usage(const char *filename)
{
	cout << "usage: " << filename << " /path/to/model.uai [OPTIONS]" << endl << endl;
	cout << "OPTIONS:" << endl;
	cout << "-h\tdisplay help information" << endl;
	cout << "-v\tverbose" << endl;
}

void
read_options(unordered_map<string,bool> &options, int argc, char *argv[])
{
	// default options
	options["verbose"] = false;
	options["help"] = false;

	for (int i = 2; i < argc; ++i) {
		string option(argv[i]);
		if (option == "-h") {
			options["help"] = true;
		}
		else if (option == "-v") {
			options["verbose"] = true;
		}
	}
}
