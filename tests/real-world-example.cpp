#include <iostream>
#include <fstream>
#include <string>
#include "../third_party/catch/single_include/catch.hpp"
#include "../third_party/json11/json11.hpp"
#include "../schema11.hpp"

using namespace json11;
using namespace schema11;
using namespace std;

//
// Pretty, consumable types
//

class ImageLayer
{
    friend Schema BindImageLayerSchema(ImageLayer &imageLayer);
private:
    
    string _Type;
    string _BlobId;
    string _Url;
    
public:
    
    string Type() const { return _Type; }
    string BlobId() const { return _BlobId; }
    string Url() const { return _Url; }
};

class Idea
{
    friend Schema BindIdeaSchema(Idea &idea);
    
private:
    std::string _Id;
    int _NumLikes;
    bool _IsRemix;
    vector<ImageLayer> _ImageLayers;
    
public:
    
    Idea() : _NumLikes(0), _IsRemix(false) {}

    string Id() const { return _Id; }
    int NumLikes() const { return _NumLikes; }
    bool IsRemix() const { return _IsRemix; }
    const vector<ImageLayer> & ImageLayers() const { return _ImageLayers; }
};

//
// Schemas
//

Schema BindImageLayerSchema(ImageLayer &imageLayer)
{
    return Schema::object {
        { "type", Schema(imageLayer._Type) },
        { "blobId", Schema(imageLayer._BlobId) },
        { "url", Schema(imageLayer._Url) }
    };
}

Schema BindIdeaSchema(Idea &idea)
{
    return Schema::object {
        { "id", Schema(idea._Id) },
        { "numLikes", Schema(idea._NumLikes) },
        { "isRemix", Schema(idea._IsRemix) },
        { "imageLayers", ArraySchema<ImageLayer>(idea._ImageLayers, &BindImageLayerSchema) }
    };
}

//
// Test Cases
//

Json LoadJson()
{
    ifstream ifs("6011983.json");
    string jsonStr;
    getline(ifs, jsonStr, (char)ifs.eof());
    string err;
    return Json::parse(jsonStr, err);
}

TEST_CASE("can parse idea")
{
    // Load the JSON object using json11 (see https://github.com/dropbox/json11)
    auto json = LoadJson();
    REQUIRE(!json.is_null());
    
    // Instantiate an Idea, bind it to the schema, and process the JSON agains the schema
    Idea idea;
    BindIdeaSchema(idea).from_json(json);
    
    // Test the values resulting from the processing are as expected. (See 6011983.json)
    REQUIRE(idea.Id() == "6011983");
    REQUIRE(idea.NumLikes() == 6);
    REQUIRE(idea.IsRemix());
    REQUIRE(idea.ImageLayers().size() == 2);
    REQUIRE(idea.ImageLayers()[0].Type() == "photo");
    REQUIRE(idea.ImageLayers()[0].BlobId() == "zPFnPEd8GFVwXAEPik0L7GAH_v36wFPo6Hs9fqzRlE4ivt7m");
    REQUIRE(idea.ImageLayers()[1].Type() == "sketch");
    REQUIRE(idea.ImageLayers()[1].BlobId() == "Klt9l_TfMYdvjuxGYwptSAus8F8JgUtd6sIYUqT3Th0ksJsU");
}