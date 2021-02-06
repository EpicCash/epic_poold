#include "jconf.hpp"
#include "encdec.h"
#include "jconf_p.hpp"
#include "rapidjson/error/en.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

const char* default_config_file = 
#include "config_json.tmpl"
;

jconfPrivate::jconfPrivate(jconf* q) : q(q)
{
}

jconf::jconf() : d(*new jconfPrivate(this))
{
}

const char* jconf::get_pid_file()
{
	return d.configValues[sPidFile]->GetString();
}

const char* jconf::get_log_file()
{
	return d.configValues[sLogFile]->GetString();
}

log_level jconf::get_log_level()
{
	static log_level val = log_level::invalid;
	
	if(val != log_level::invalid)
		return val;

	const char* valstr = d.configValues[sLogLevel]->GetString();
	if(strcmp(valstr, "error") == 0)
		val = log_level::error;
	else if(strcmp(valstr, "warn") == 0)
		val = log_level::warn;
	else if(strcmp(valstr, "info") == 0)
		val = log_level::info;
	else if(strcmp(valstr, "debug_hi") == 0)
		val = log_level::debug_hi;
	else if(strcmp(valstr, "debug_lo") == 0)
		val = log_level::debug_lo;
	else
		val = log_level::invalid;
	return val;
}

const char* jconf::get_db_hostname()
{
	return d.configValues[sDbHostname]->GetString();
}

const char* jconf::get_db_port()
{
	return d.configValues[sDbPort]->GetString();
}

const char* jconf::get_db_name()
{
	return d.configValues[sDbName]->GetString();
}

const char* jconf::get_db_username()
{
	return d.configValues[sDbUsername]->GetString();
}

const char* jconf::get_db_password()
{
	return d.configValues[sDbPassword]->GetString();
}

const char* jconf::get_node_hostname()
{
	return d.configValues[sNodeHostname]->GetString();
}

const char* jconf::get_node_port()
{
	return d.configValues[sNodePort]->GetString();
}

const char* jconf::get_node_username()
{
	return d.configValues[sNodeUsername]->GetString();
}

const char* jconf::get_node_password()
{
	return d.configValues[sNodePassword]->GetString();
}

bool jconf::daemonize()
{
	return d.configValues[bDaemonize]->GetBool();
}

uint16_t jconf::get_server_port()
{
	return d.configValues[iPlainPort]->GetUint();
}

uint16_t jconf::get_server_tls_port()
{
	return d.configValues[iTlsPort]->GetUint();
}

const char* jconf::get_tls_cert_filename()
{
	return d.configValues[sTlsCert]->GetString();
}

const char* jconf::get_tls_cipher_list()
{
	return d.configValues[sTlsCipers]->GetString();
}

size_t jconf::get_template_timeout()
{
	return d.configValues[iTemplateTimeout]->GetUint();
}

inline std::string read_file(const char* filename, bool& error)
{
	struct stat sb;
	if(stat(filename, &sb) == -1)
	{
		fprintf(stderr, "Config file %s not found. Will create it and exit. Adjust the settings and run again.\n", filename);
		int fd = open(filename, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR);
		if(fd == -1)
		{
			fputs("Panic! Could not open config file for writing!\n", stderr);
			error = true;
			return "";
		}

		ssize_t conf_len = strlen(default_config_file);
		if(write(fd, default_config_file, conf_len) != conf_len)
		{
			fputs("Panic! Writing to config file failed!\n", stderr);
			error = true;
			return "";
		}
		close(fd);
		error = true;
		return "";
	}

	std::string conf(sb.st_size, '\0');
	int fd = open(filename, O_RDONLY);
	if(fd == -1)
	{
		fputs("Panic! Could not open config file for reading!\n", stderr);
		error = true;
		return "";
	}

	ssize_t ln = read(fd, &conf[0], sb.st_size);
	close(fd);
	if(ln != sb.st_size)
	{
		fputs("Panic! Reading config file failed!\n", stderr);
		error = true;
		return "";
	}

	return conf;
}

bool jconf::parse_config(const char* filename)
{
	bool rd_error;
	std::string file = read_file(filename, rd_error);

	if(rd_error)
		return false;

	d.jsonDoc.Parse<kParseCommentsFlag | kParseTrailingCommasFlag>(file.c_str(), file.length());

	if(d.jsonDoc.HasParseError())
	{
		fprintf(stderr, "JSON config parse error(offset %zu): %s\n", d.jsonDoc.GetErrorOffset(), GetParseError_En(d.jsonDoc.GetParseError()));
		return false;
	}

	if(!d.jsonDoc.IsObject())
	{
		fputs("JSON root is not an object.\n", stderr);
		return false;
	}

	for(size_t i = 0; i < iConfigCnt; i++)
	{
		if(oConfigValues[i].iName != i)
		{
			fputs("Code error. oConfigValues are not in order.\n", stderr);
			return false;
		}

		d.configValues[i] = GetObjectMember(d.jsonDoc, oConfigValues[i].sName);

		if(d.configValues[i] == nullptr)
		{
			fprintf(stderr, "Invalid config file. Missing value \"%s\".\n", oConfigValues[i].sName);
			return false;
		}

		if(!checkType(d.configValues[i]->GetType(), oConfigValues[i].iType))
		{
			fprintf(stderr, "Invalid config file. Value \"%s\" has unexpected type.\n", oConfigValues[i].sName);
			return false;
		}

		if((oConfigValues[i].flagConstraints & flag_unsigned) != 0 && !check_constraint_unsigned(d.configValues[i]))
		{
			fprintf(stderr, "Invalid config file. Value \"%s\" needs to be unsigned.\n", oConfigValues[i].sName);
			return false;
		}

		if((oConfigValues[i].flagConstraints & flag_u16bit) != 0 && !check_constraint_u16bit(d.configValues[i]))
		{
			fprintf(stderr, "Invalid config file. Value \"%s\" needs to be lower than 65535.\n", oConfigValues[i].sName);
			return false;
		}
	}

	if(get_log_level() == log_level::invalid)
	{
		fprintf(stderr, "Invalid log_level, allowed values are \"error\", \"warn\", \"info\", \"debug_hi\", \"debug_lo\"\n");
		return false;
	}

	return true;
}
