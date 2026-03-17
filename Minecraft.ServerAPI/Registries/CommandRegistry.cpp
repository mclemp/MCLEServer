#include "stdafx.h"
#include "CommandRegistry.h"

//std::vector<std::shared_ptr<Command>> CommandRegistry::registeredCommands;

//bool CommandRegistry::OnConsoleCommand(MinecraftServer* Instance, const wstring& rawCommand) {
//	return false;
//}
/*

	wstring command = trimString(rawCommand);
	if (command.empty())
		return true;

	if (command[0] == L'/')
	{
		command = trimString(command.substr(1));
	}

	vector<wstring> tokens = SplitConsoleCommand(command);
	if (tokens.empty())
		return true;

	const wstring action = toLower(tokens[0]);
	PlayerList *playerList = server->getPlayers();

	if (action == L"help" || action == L"?")
	{
		server->info(L"Commands: help, stop, list, say <message>, save-all, time <set day|night|ticks|add ticks>, weather <clear|rain|thunder> [seconds], tp <player> <target>, give <player> <itemId> [amount] [aux], enchant <player> <enchantId> [level], kill <player>");
		return true;
	}

	if (action == L"stop")
	{
		server->info(L"Stopping server...");
		MinecraftServer::HaltServer();
		return true;
	}

	if (action == L"list")
	{
		wstring playerNames = (playerList != nullptr) ? playerList->getPlayerNames() : L"";
		if (playerNames.empty()) playerNames = L"(none)";
		server->info(L"Players (" + std::to_wstring((playerList != nullptr) ? playerList->getPlayerCount() : 0) + L"): " + playerNames);
		return true;
	}

	if (action == L"say")
	{
		if (tokens.size() < 2)
		{
			server->warn(L"Usage: say <message>");
			return false;
		}

		wstring message = L"[Server] " + JoinConsoleCommandTokens(tokens, 1);
		if (playerList != nullptr)
		{
			playerList->broadcastAll(std::make_shared<ChatPacket>(message));
		}
		server->info(message);
		return true;
	}

	if (action == L"save-all")
	{
		if (playerList != nullptr)
		{
			playerList->saveAll(nullptr, false);
		}
		server->info(L"World saved.");
		return true;
	}

	if (action == L"time")
	{
		if (tokens.size() < 2)
		{
			server->warn(L"Usage: time set <day|night|ticks> | time add <ticks>");
			return false;
		}

		if (toLower(tokens[1]) == L"add")
		{
			if (tokens.size() < 3)
			{
				server->warn(L"Usage: time add <ticks>");
				return false;
			}

			int delta = 0;
			if (!TryParseIntValue(tokens[2], delta))
			{
				server->warn(L"Invalid tick value: " + tokens[2]);
				return false;
			}

			for (unsigned int i = 0; i < server->levels.length; ++i)
			{
				if (server->levels[i] != nullptr)
				{
					server->levels[i]->setDayTime(server->levels[i]->getDayTime() + delta);
				}
			}

			server->info(L"Added " + std::to_wstring(delta) + L" ticks.");
			return true;
		}

		wstring timeValue = toLower(tokens[1]);
		if (timeValue == L"set")
		{
			if (tokens.size() < 3)
			{
				server->warn(L"Usage: time set <day|night|ticks>");
				return false;
			}
			timeValue = toLower(tokens[2]);
		}

		int targetTime = 0;
		if (timeValue == L"day")
		{
			targetTime = 0;
		}
		else if (timeValue == L"night")
		{
			targetTime = 12500;
		}
		else if (!TryParseIntValue(timeValue, targetTime))
		{
			server->warn(L"Invalid time value: " + timeValue);
			return false;
		}

		SetAllLevelTimes(server, targetTime);
		server->info(L"Time set to " + std::to_wstring(targetTime) + L".");
		return true;
	}

	if (action == L"weather")
	{
		if (tokens.size() < 2)
		{
			server->warn(L"Usage: weather <clear|rain|thunder> [seconds]");
			return false;
		}

		int durationSeconds = 600;
		if (tokens.size() >= 3 && !TryParseIntValue(tokens[2], durationSeconds))
		{
			server->warn(L"Invalid duration: " + tokens[2]);
			return false;
		}

		if (server->levels[0] == nullptr)
		{
			server->warn(L"The overworld is not loaded.");
			return false;
		}

		LevelData *levelData = server->levels[0]->getLevelData();
		int duration = durationSeconds * SharedConstants::TICKS_PER_SECOND;
		levelData->setRainTime(duration);
		levelData->setThunderTime(duration);

		wstring weather = toLower(tokens[1]);
		if (weather == L"clear")
		{
			levelData->setRaining(false);
			levelData->setThundering(false);
		}
		else if (weather == L"rain")
		{
			levelData->setRaining(true);
			levelData->setThundering(false);
		}
		else if (weather == L"thunder")
		{
			levelData->setRaining(true);
			levelData->setThundering(true);
		}
		else
		{
			server->warn(L"Usage: weather <clear|rain|thunder> [seconds]");
			return false;
		}

		server->info(L"Weather set to " + weather + L".");
		return true;
	}

	if (action == L"tp" || action == L"teleport")
	{
		if (tokens.size() < 3)
		{
			server->warn(L"Usage: tp <player> <target>");
			return false;
		}

		shared_ptr<ServerPlayer> subject = FindPlayerByName(playerList, tokens[1]);
		shared_ptr<ServerPlayer> destination = FindPlayerByName(playerList, tokens[2]);
		if (subject == nullptr)
		{
			server->warn(L"Unknown player: " + tokens[1]);
			return false;
		}
		if (destination == nullptr)
		{
			server->warn(L"Unknown player: " + tokens[2]);
			return false;
		}
		if (subject->level->dimension->id != destination->level->dimension->id || !subject->isAlive())
		{
			server->warn(L"Teleport failed because the players are not in the same dimension or the source player is dead.");
			return false;
		}

		subject->ride(nullptr);
		subject->connection->teleport(destination->x, destination->y, destination->z, destination->yRot, destination->xRot);
		server->info(L"Teleported " + subject->getName() + L" to " + destination->getName() + L".");
		return true;
	}

	if (action == L"give")
	{
		if (tokens.size() < 3)
		{
			server->warn(L"Usage: give <player> <itemId> [amount] [aux]");
			return false;
		}

		shared_ptr<ServerPlayer> player = FindPlayerByName(playerList, tokens[1]);
		if (player == nullptr)
		{
			server->warn(L"Unknown player: " + tokens[1]);
			return false;
		}

		int itemId = 0;
		int amount = 1;
		int aux = 0;
		if (!TryParseIntValue(tokens[2], itemId))
		{
			server->warn(L"Invalid item id: " + tokens[2]);
			return false;
		}
		if (tokens.size() >= 4 && !TryParseIntValue(tokens[3], amount))
		{
			server->warn(L"Invalid amount: " + tokens[3]);
			return false;
		}
		if (tokens.size() >= 5 && !TryParseIntValue(tokens[4], aux))
		{
			server->warn(L"Invalid aux value: " + tokens[4]);
			return false;
		}
		if (itemId <= 0 || Item::items[itemId] == nullptr)
		{
			server->warn(L"Unknown item id: " + std::to_wstring(itemId));
			return false;
		}
		if (amount <= 0)
		{
			server->warn(L"Amount must be positive.");
			return false;
		}

		shared_ptr<ItemInstance> itemInstance(new ItemInstance(itemId, amount, aux));
		shared_ptr<ItemEntity> drop = player->drop(itemInstance);
		if (drop != nullptr)
		{
			drop->throwTime = 0;
		}
		server->info(L"Gave item " + std::to_wstring(itemId) + L" x" + std::to_wstring(amount) + L" to " + player->getName() + L".");
		return true;
	}

	if (action == L"enchant")
	{
		if (tokens.size() < 3)
		{
			server->warn(L"Usage: enchant <player> <enchantId> [level]");
			return false;
		}

		shared_ptr<ServerPlayer> player = FindPlayerByName(playerList, tokens[1]);
		if (player == nullptr)
		{
			server->warn(L"Unknown player: " + tokens[1]);
			return false;
		}

		int enchantmentId = 0;
		int enchantmentLevel = 1;
		if (!TryParseIntValue(tokens[2], enchantmentId))
		{
			server->warn(L"Invalid enchantment id: " + tokens[2]);
			return false;
		}
		if (tokens.size() >= 4 && !TryParseIntValue(tokens[3], enchantmentLevel))
		{
			server->warn(L"Invalid enchantment level: " + tokens[3]);
			return false;
		}

		shared_ptr<ItemInstance> selectedItem = player->getSelectedItem();
		if (selectedItem == nullptr)
		{
			server->warn(L"The player is not holding an item.");
			return false;
		}

		Enchantment *enchantment = Enchantment::enchantments[enchantmentId];
		if (enchantment == nullptr)
		{
			server->warn(L"Unknown enchantment id: " + std::to_wstring(enchantmentId));
			return false;
		}
		if (!enchantment->canEnchant(selectedItem))
		{
			server->warn(L"That enchantment cannot be applied to the selected item.");
			return false;
		}

		if (enchantmentLevel < enchantment->getMinLevel()) enchantmentLevel = enchantment->getMinLevel();
		if (enchantmentLevel > enchantment->getMaxLevel()) enchantmentLevel = enchantment->getMaxLevel();

		if (selectedItem->hasTag())
		{
			ListTag<CompoundTag> *enchantmentTags = selectedItem->getEnchantmentTags();
			if (enchantmentTags != nullptr)
			{
				for (int i = 0; i < enchantmentTags->size(); i++)
				{
					int type = enchantmentTags->get(i)->getShort((wchar_t *)ItemInstance::TAG_ENCH_ID);
					if (Enchantment::enchantments[type] != nullptr && !Enchantment::enchantments[type]->isCompatibleWith(enchantment))
					{
						server->warn(L"That enchantment conflicts with an existing enchantment on the selected item.");
						return false;
					}
				}
			}
		}

		selectedItem->enchant(enchantment, enchantmentLevel);
		server->info(L"Enchanted " + player->getName() + L"'s held item with " + std::to_wstring(enchantmentId) + L" " + std::to_wstring(enchantmentLevel) + L".");
		return true;
	}

	if (action == L"kill")
	{
		if (tokens.size() < 2)
		{
			server->warn(L"Usage: kill <player>");
			return false;
		}

		shared_ptr<ServerPlayer> player = FindPlayerByName(playerList, tokens[1]);
		if (player == nullptr)
		{
			server->warn(L"Unknown player: " + tokens[1]);
			return false;
		}

		player->hurt(DamageSource::outOfWorld, 3.4e38f);
		server->info(L"Killed " + player->getName() + L".");
		return true;
	}

	server->warn(L"Unknown command: " + command);
	return false;
	*/
