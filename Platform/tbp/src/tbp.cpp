#include <sqlite/sqlite.h>

class data_provider
{
public:
	virtual sqlite::connection::ptr create_data_provider() = 0;
};

int main()
{
    return 0;
}