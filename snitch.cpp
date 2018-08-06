#include <yaml-cpp/yaml.h>
#include <twitter.h>
#include <fstream>
#include <thread>
#include <chrono>
#include <iostream>
#include <algorithm>

// Sync followers every 4 hours.
const int CHECK_FOLLOWERS_EVERY = 4 * 60 / 5;

int main(int argc, char** argv)
{
  if (argc != 2)
  {
    std::cout << "usage: snitch [configfile]" << std::endl;
    return -1;
  }

  std::string configfile(argv[1]);
  YAML::Node config = YAML::LoadFile(configfile);

  twitter::auth auth(
    config["consumer_key"].as<std::string>(),
    config["consumer_secret"].as<std::string>(),
    config["access_key"].as<std::string>(),
    config["access_secret"].as<std::string>());

  std::ifstream img_file(config["image"].as<std::string>());
  img_file.seekg(0, std::ios::end);
  size_t img_len = img_file.tellg();
  char* img_buf = new char[img_len];
  img_file.seekg(0, std::ios::beg);
  img_file.read(img_buf, img_len);
  img_file.close();

  std::vector<std::string> triggers {
    "calling the cops",
    "calling the police",
    "call the cops",
    "call the police"
  };

  auto startedTime = std::chrono::system_clock::now();

  // Initialize the client
  twitter::client client(auth);

  std::set<twitter::user_id> friends;
  int followerTimeout = 0;

  for (;;)
  {
    if (followerTimeout == 0)
    {
      // Sync friends with followers.
      try
      {
        friends = client.getFriends();

        std::set<twitter::user_id> followers = client.getFollowers();

        std::list<twitter::user_id> oldFriends;
        std::set_difference(
          std::begin(friends),
          std::end(friends),
          std::begin(followers),
          std::end(followers),
          std::back_inserter(oldFriends));

        std::set<twitter::user_id> newFollowers;
        std::set_difference(
          std::begin(followers),
          std::end(followers),
          std::begin(friends),
          std::end(friends),
          std::inserter(newFollowers, std::begin(newFollowers)));

        for (twitter::user_id f : oldFriends)
        {
          client.unfollow(f);
        }

        std::list<twitter::user> newFollowerObjs =
          client.hydrateUsers(std::move(newFollowers));

        for (twitter::user f : newFollowerObjs)
        {
          if (!f.isProtected())
          {
            client.follow(f);
          }
        }
      } catch (const twitter::twitter_error& error)
      {
        std::cout << "Twitter error while syncing followers: " << error.what()
          << std::endl;
      }

      followerTimeout = CHECK_FOLLOWERS_EVERY;
    }

    followerTimeout--;

    try
    {
      // Poll the timeline.
      std::list<twitter::tweet> tweets = client.getHomeTimeline().poll();

      for (twitter::tweet& tweet : tweets)
      {
        auto createdTime =
          std::chrono::system_clock::from_time_t(tweet.getCreatedAt());

        if (
          // Only monitor people you are following
          friends.count(tweet.getAuthor().getID()) &&
          // Ignore tweets from before the bot started up
          createdTime > startedTime)
        {
          std::string orig = tweet.getText();
          std::string canonical;

          std::transform(
            std::begin(orig),
            std::end(orig),
            std::back_inserter(canonical),
            [] (char ch) {
              return std::tolower(ch);
            });

          for (const std::string& trigger : triggers)
          {
            if (canonical.find(trigger) != std::string::npos)
            {
              std::cout << "Calling the cops on @"
                << tweet.getAuthor().getScreenName() << std::endl;

              try
              {
                long media_id =
                  client.uploadMedia(
                    "image/jpeg",
                    static_cast<const char*>(img_buf),
                    img_len);

                client.replyToTweet(
                  tweet.generateReplyPrefill(client.getUser()),
                  tweet.getID(),
                  {media_id});
              } catch (const twitter::twitter_error& e)
              {
                std::cout << "Twitter error while tweeting: " << e.what()
                  << std::endl;
              }

              break;
            }
          }
        }
      }
    } catch (const twitter::rate_limit_exceeded&)
    {
      // Wait out the rate limit (10 minutes here and 5 below = 15).
      std::this_thread::sleep_for(std::chrono::minutes(10));
    }

    // We can poll the timeline at most once every five minutes.
    std::this_thread::sleep_for(std::chrono::minutes(5));
  }
}
