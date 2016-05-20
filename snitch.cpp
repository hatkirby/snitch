#include <yaml-cpp/yaml.h>
#include <twitter.h>
#include <fstream>
#include <thread>
#include <chrono>

int main(int argc, char** argv)
{
  YAML::Node config = YAML::LoadFile("config.yml");
    
  twitter::auth auth;
  auth.setConsumerKey(config["consumer_key"].as<std::string>());
  auth.setConsumerSecret(config["consumer_secret"].as<std::string>());
  auth.setAccessKey(config["access_key"].as<std::string>());
  auth.setAccessSecret(config["access_secret"].as<std::string>());
  
  std::ifstream img_file("image.jpg");
  img_file.seekg(0, std::ios::end);
  size_t img_len = img_file.tellg();
  char* img_buf = new char[img_len];
  img_file.seekg(0, std::ios::beg);
  img_file.read(img_buf, img_len);
  img_file.close();
  
  twitter::client client(auth);
  client.setUserStreamNotifyCallback([&] (twitter::notification n) {
    if (n.getType() == twitter::notification::type::tweet)
    {
      std::string orig = n.getTweet().getText();
      std::string canonical;
      std::transform(std::begin(orig), std::end(orig), std::back_inserter(canonical), [] (char ch) {
        return std::tolower(ch);
      });
      
      if (canonical.find("calling the cops") != std::string::npos)
      {
        std::cout << "Calling the cops on @" << n.getTweet().getAuthor().getScreenName() << std::endl;
        
        long media_id;
        twitter::response resp = client.uploadMedia("image/jpeg", (const char*) img_buf, img_len, media_id);
        if (resp != twitter::response::ok)
        {
          std::cout << "Twitter error while uploading image: " << resp << std::endl;
        } else {
          twitter::tweet tw;
          resp = client.updateStatus("@" + n.getTweet().getAuthor().getScreenName(), tw, n.getTweet(), {media_id});
          if (resp != twitter::response::ok)
          {
            std::cout << "Twitter error while tweeting: " << resp << std::endl;
          }
        }
      }
    } else if (n.getType() == twitter::notification::type::followed)
    {
      twitter::response resp = client.follow(n.getUser());
      if (resp != twitter::response::ok)
      {
        std::cout << "Twitter error while following @" << n.getUser().getScreenName() << ": " << resp << std::endl;
      }
    }
  });

  std::this_thread::sleep_for(std::chrono::minutes(1));
  
  std::cout << "Starting streaming" << std::endl;
  client.startUserStream();
  for (;;)
  {
    std::this_thread::sleep_for(std::chrono::hours(4));
    
    std::set<twitter::user_id> friends;
    std::set<twitter::user_id> followers;
    twitter::response resp = client.getFriends(friends);
    if (resp == twitter::response::ok)
    {
      resp = client.getFollowers(followers);
      if (resp == twitter::response::ok)
      {
        auto frit = std::begin(friends);
        auto foit = std::begin(followers);
        
        while (frit != std::end(friends))
        {
          auto match = std::mismatch(frit, std::end(friends), foit);
          if (match.first != std::end(friends))
          {
            resp = client.unfollow(*match.first);
            if (resp != twitter::response::ok)
            {
              std::cout << "Twitter error while unfollowing" << std::endl;
            }
            
            frit = match.first;
            frit++;
          }
        }
      } else {
        std::cout << "Twitter error while getting followers: " << resp << std::endl;
      }
    } else {
      std::cout << "Twitter error while getting friends: " << resp << std::endl;
    }
  }
  
  client.stopUserStream();
}
