#include "scenarioInfo.h"
#include "resources.h"

ScenarioInfo::ScenarioInfo(string filename)
{
    this->filename = filename;
    // Support paths with folder prefix (e.g. "folder/scenario_foo.lua").
    string basename = filename;
    int last_slash = filename.rfind("/");
    if (last_slash < 0)
        last_slash = filename.rfind("\\");
    if (last_slash >= 0)
        basename = filename.substr(last_slash + 1);
    if (basename.startswith("scenario_") && basename.endswith(".lua"))
        name = basename.substr(9, -4);
    else
        name = basename;

    P<ResourceStream> stream = getResourceStream(filename);
    if (!stream)
    {
        LOG(ERROR) << "Scenario not found: " << filename;
        return;
    }

    string key;
    string value;
    while(stream->tell() < stream->getSize())
    {
        string line = stream->readLine().strip();
        // Get the scenario meta-data.
        if (!line.startswith("--"))
            break;
        if (line.startswith("---"))
        {
            line = line.substr(3).strip();
            value = value + "\n" + line;
        }else{
            line = line.substr(2).strip();
            if (line.find(":") < 0)
            {
                key = "";
                continue;
            }
            addKeyValue(key, value);
            key = line.substr(0, line.find(":")).strip();
            value = line.substr(line.find(":") + 1).strip();
        }
    }
    addKeyValue(key, value);
    if (type == "")
        LOG(WARNING) << "No scenario type for: " << filename;
}

void ScenarioInfo::addKeyValue(string key, string value)
{
    if (key == "")
        return;
    if (key.lower() == "name")
    {
        name = value;
    }
    else if (key.lower() == "description")
    {
        description = value;
    }
    else if (key.lower() == "author")
    {
        author = value;
    }
    else if (key.lower() == "type")
    {
        type = value;
    }
    else if (key.lower().startswith("variation[") && key.endswith("]"))
    {
        variations.emplace_back(key.substr(10, -1), value);
    }else{
        LOG(WARNING) << "Unknown scenario meta data: " << key << ": " << value;
    }
}
