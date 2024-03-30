#include <swiftly/swiftly.h>
#include <swiftly/server.h>
#include <swiftly/database.h>
#include <swiftly/commands.h>
#include <swiftly/configuration.h>
#include <swiftly/logger.h>
#include <swiftly/timers.h>
#include <swiftly/gameevents.h>
#include <swiftly/http.h>

Server *server = nullptr;
PlayerManager *g_playerManager = nullptr;
Database *db = nullptr;
Commands *commands = nullptr;
Configuration *config = nullptr;
Logger *logger = nullptr;
Timers *timers = nullptr;
HTTP *http = nullptr;
JSON *json = nullptr;

void OnProgramLoad(const char *pluginName, const char *mainFilePath)
{
    Swiftly_Setup(pluginName, mainFilePath);

    server = new Server();
    g_playerManager = new PlayerManager();
    commands = new Commands(pluginName);
    config = new Configuration();
    logger = new Logger(mainFilePath, pluginName);
    timers = new Timers();
    http = new HTTP();
    json = new JSON();
}

void OnClientFullConnected(Player *player)
{

    if (player && !player->IsFakeClient())
    {

        HTTPRequest *hRequest = http->GenerateRequest("https://open.faceit.com/");
        char authHeader[256];
        std::sprintf(authHeader, "Bearer %s", config->Fetch<const char *>("swiftly_faceitlevel.apikey"));
        hRequest->AddHeader("Authorization", authHeader);
        hRequest->SetFollowRedirect(true);

        char url[100];
        snprintf(url, sizeof(url), "/data/v4/players?game=cs2&game_player_id=%llu", player->GetSteamID());
        hRequest->Get(url);

        int statusCode = hRequest->GetStatusCode();

        if (statusCode == 200 || statusCode == 404)
        {
            const char *body = hRequest->GetBody();
            print("%s\n", hRequest->GetBody());
            JSONObject *root = json->Parse(body);
            if (root)
            {
                rapidjson::Document &document = root->document;
                if (document.HasMember("games"))
                {
                    const rapidjson::Value &games = document["games"];
                    if (games.IsObject() && games.HasMember("cs2"))
                    {
                        const rapidjson::Value &cs2 = games["cs2"];
                        if (cs2.IsObject() && cs2.HasMember("faceit_elo"))
                        {
                            int elo = cs2["faceit_elo"].GetInt();
                            player->vars->Set("elo", elo);
                        }
                    }
                }
            }
        }

        int MinimumElo = config->Fetch<int>("swiftly_faceitlevel.minimumelo");
        int MaximumElo = config->Fetch<int>("swiftly_faceitlevel.maximumelo");

        if (player->vars->Get<int>("elo") < MinimumElo || player->vars->Get<int>("elo") > MaximumElo)
        {
            player->Drop(NETWORK_DISCONNECT_KICKED);
        }
    }
}

void OnPluginStart()
{
}

void OnPluginStop()
{
}

const char *GetPluginAuthor()
{
    return "";
}

const char *GetPluginVersion()
{
    return "1.0.0";
}

const char *GetPluginName()
{
    return "swiftly_faceitlevel";
}

const char *GetPluginWebsite()
{
    return "";
}