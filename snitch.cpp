#include <yaml-cpp/yaml.h>
#include <twitter.h>
#include <fstream>
#include <thread>
#include <chrono>
#include <iostream>
#include <algorithm>

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
  
  std::vector<std::string> triggers {
    "calling the cops",
    "calling the police"
  };
  
  // Initialize the client
  twitter::client client(auth);
  std::this_thread::sleep_for(std::chrono::minutes(1));
  
  // Start streaming
  std::cout << "Starting streaming" << std::endl;
  std::set<twitter::user_id> streamed_friends;
  twitter::stream userStream(client, [&] (const twitter::notification& n) {
    if (n.getType() == twitter::notification::type::friends)
    {
      streamed_friends = n.getFriends();
    } else if (n.getType() == twitter::notification::type::follow)
    {
      streamed_friends.insert(n.getUser().getID());
    } else if (n.getType() == twitter::notification::type::unfollow)
    {
      streamed_friends.erase(n.getUser().getID());
    } else if (n.getType() == twitter::notification::type::tweet)
    {
      // Only monitor people you are following
      if (streamed_friends.count(n.getTweet().getAuthor().getID()) == 1)
      {
        std::string orig = n.getTweet().getText();
        std::string canonical;
        std::transform(std::begin(orig), std::end(orig), std::back_inserter(canonical), [] (char ch) {
          return std::tolower(ch);
        });
        
        for (auto trigger : triggers)
        {
          if (canonical.find(trigger) != std::string::npos)
          {
            std::cout << "Calling the cops on @" << n.getTweet().getAuthor().getScreenName() << std::endl;
        
            try
            {
              long media_id = client.uploadMedia("image/jpeg", (const char*) img_buf, img_len);
              client.replyToTweet(n.getTweet().generateReplyPrefill(), n.getTweet().getID(), {media_id});
            } catch (const twitter::twitter_error& e)
            {
              std::cout << "Twitter error: " << e.what() << std::endl;
            }
            
            break;
          }
        }
      }
    } else if (n.getType() == twitter::notification::type::followed)
    {
      try
      {
        n.getUser().follow();
      } catch (const twitter::twitter_error& e)
      {
        std::cout << "Twitter error while following @" << n.getUser().getScreenName() << ": " << e.what() << std::endl;
      }
    }
  }, true, true);

  for (;;)
  {
    std::this_thread::sleep_for(std::chrono::minutes(1));
    
    try
    {
      std::set<twitter::user_id> friends = client.getUser().getFriends();
      std::set<twitter::user_id> followers = client.getUser().getFollowers();

      std::list<twitter::user_id> old_friends, new_followers;
      std::set_difference(std::begin(friends), std::end(friends), std::begin(followers), std::end(followers), std::back_inserter(old_friends));
      std::set_difference(std::begin(followers), std::end(followers), std::begin(friends), std::end(friends), std::back_inserter(new_followers));

      for (auto f : old_friends)
      {
        client.unfollow(f);
      }
      
      for (auto f : new_followers)
      {
        client.follow(f);
      }
    } catch (const twitter::twitter_error& e)
    {
      std::cout << "Twitter error: " << e.what() << std::endl;
    }
    
    std::this_thread::sleep_for(std::chrono::hours(4));
  }
}
