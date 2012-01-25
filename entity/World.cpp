/* 
 * File:   World.cpp
 * Author: cyril
 * 
 * Created on January 2, 2012, 6:47 PM
 */

#include "World.h"
#include <stdio.h>
#include <iostream>

#include "../ai/AiManager.h"
#include "../entity/Block.h"
#include "../entity/Character.h"
#include "../entity/Goal.h"

World::World() {
    player = new Character(this);
    player->setColor(sf::Color(0, 0, 255));
    width = 0;
    height = 0;
    completed = false;
    ai = new AiManager(this);
}

World::~World() {
    clean();
    delete ai;
    delete player;

}

void World::clean() {
    delete ai;
    std::vector<Block*>::iterator it;
    for (it = blocks.begin(); it != blocks.end(); it++)
        free(*it);

    std::vector<Goal*>::iterator git;
    for (git = goals.begin(); git != goals.end(); git++)
        free(*git);

    std::vector<Character*>::iterator enm;
    for (enm = enemies.begin(); enm != enemies.end(); enm++)
        free(*enm);

    blocks.clear();
    goals.clear();
    enemies.clear();
    completed = false;
    ai = new AiManager(this);
}

void World::draw(sf::RenderTarget* rt) {

    std::vector<Block*>::iterator it;
    for (it = blocks.begin(); it != blocks.end(); it++)
        (*it)->draw(rt);

    std::vector<Goal*>::iterator git;
    for (git = goals.begin(); git != goals.end(); git++)
        (*git)->draw(rt);

    std::vector<Character*>::iterator enm;
    for (enm = enemies.begin(); enm != enemies.end(); enm++)
        (*enm)->draw(rt);

    player->draw(rt);
}

int World::update(sf::Time frametime, Input input) {
    std::vector<Block*>::iterator it;
    for (it = blocks.begin(); it != blocks.end(); it++) {
        (*it)->update(frametime);
        (*it)->setColor(sf::Color(255, 255, 255));
    }
    player->update(frametime, input);

    std::vector<Character*>::iterator cit;
    for (cit = enemies.begin(); cit != enemies.end(); cit++)
        (*cit)->update(frametime, ai->update((*cit), frametime));

    std::vector<Goal*>::iterator git;
    for (git = goals.begin(); git != goals.end(); git++)
        if (player->getBbox().Intersects((*git)->getBbox())) {
            delete(*git);
            git = goals.erase(git);
            git--;
        }

    if (goals.size() == 0 && !completed) {
        completed = true;
        for (it = blocks.begin(); it != blocks.end(); it++)
            if ((*it)->getType() == Block::ENDLADDER)
                (*it)->setActive(true);
    }

    if (completed && player->getPosition().y <= 30) 
        return World::WIN;
     else
        return World::NOTHING;
}

void World::loadFromFile(std::string filename) {
    clean();
    FILE *f = fopen(filename.c_str(), "r");
    if (!f) {
        std::cerr << "Can't load file: " << filename << std::endl;
        return;
    }

    fscanf(f, "d%dx%d\n", &width, &height);

    width += 2;
    height += 2;
    for (int i = 0; i < width; i++)
        addBlock(i, 0, Block::CEMENT);

    for (int j = 1; j < height - 1; j++) {
        addBlock(0, j, Block::CEMENT);

        for (int i = 1; i < width - 1; i++) {

            int value;
            fscanf(f, "%1d", &value);

            //BLOCK
            addBlock(i, j, value);

            if (value == 9)
                player->setPosition(sf::Vector2f(i * Block::WIDTH, j * Block::HEIGHT));

            else if (value == 8) {
                Character* en = new Character(this);
                en->setPosition(sf::Vector2f(i * Block::WIDTH, j * Block::HEIGHT));
                en->setSpeed(sf::Vector2f(175, 150));
                enemies.push_back(en);
                ai->addAgent(en);
            } else if (value == 7) {
                Goal* g = new Goal();
                g->setPosition(sf::Vector2f(i * Block::WIDTH, j * Block::HEIGHT));
                goals.push_back(g);
            }
        }
        fscanf(f, "\n");
        addBlock(width - 1, j, Block::CEMENT);
    }
    for (int i = 0; i < width; i++)
        addBlock(i, height - 1, Block::CEMENT);
    fclose(f);
}

Block* World::getCollidingSolid(sf::FloatRect bbox) {

    for (int j = bbox.Top / Block::HEIGHT; j <= (bbox.Top + bbox.Height) / Block::HEIGHT; j++)
        for (int i = bbox.Left / Block::WIDTH; i <= (bbox.Left + bbox.Width) / Block::WIDTH; i++) {
            Block *candidate = getBlock(i, j);
            if (candidate->getBbox().Intersects(bbox) && candidate->isSolid()) {
                return candidate;

            }
        }

    return NULL;
}

Character* World::getCollidingEnnemy(sf::FloatRect bbox) {
std::vector<Character*>::iterator enm;
    for (enm = enemies.begin(); enm != enemies.end(); enm++)
        if((*enm)->getBbox().Intersects(bbox))
            return *enm;
    
    return NULL;
}

Block* World::getCollidingLadder(sf::FloatRect bbox) {

    for (int j = bbox.Top / Block::HEIGHT; j <= (bbox.Top + bbox.Height) / Block::HEIGHT; j++)
        for (int i = bbox.Left / Block::WIDTH; i <= (bbox.Left + bbox.Width) / Block::WIDTH; i++) {

            Block *candidate = getBlock(i, j);
            if (candidate->getBbox().Intersects(bbox) && candidate->isLadder()) {
                return candidate;

            }
        }

    return NULL;
}

Block* World::getCollidingRope(sf::FloatRect bbox) {

    for (int j = bbox.Top / Block::HEIGHT; j <= (bbox.Top + bbox.Height) / Block::HEIGHT; j++)
        for (int i = bbox.Left / Block::WIDTH; i <= (bbox.Left + bbox.Width) / Block::WIDTH; i++) {

            Block *candidate = getBlock(i, j);
            if (candidate->getBbox().Intersects(bbox) && candidate->isRope()) {
                return candidate;

            }
        }

    return NULL;
}

sf::Vector2f World::getSize() {
    return sf::Vector2f(width * Block::WIDTH, height * Block::HEIGHT);
}

bool World::isUnderRope(int x, int y) {
    for (int j = y - 1; j > 0; j--) {
        if (getBlock(x, j)->getType() == Block::ROPE)
            return true;
        else if (getBlock(x, j)->isSolid())
            break;
    }
    return false;
}

std::list<Block*> World::getNeighbors(int x, int y) {
    std::list<Block*> value;
    Block* current = getBlock(x, y);
    bool isSolidUnderCurrent = false;
    if (y + 1 < height)
        isSolidUnderCurrent = getBlock(x, y + 1)->isSolid() || getBlock(x, y + 1)->isLadder();

    int xs[] = {x, x - 1, x + 1, x};
    int ys[] = {y - 1, y, y, y + 1};


    for (int i = 0; i < 4; i++) {

        if (xs[i] < 0 || xs[i] >= width || ys[i] < 0 || ys[i] >= height)
            continue;

        Block* candidate = getBlock(xs[i], ys[i]);

        if ((candidate->isLadder() || candidate->isRope()) && (i == 1 || i == 2))
            value.push_back(candidate);
        else if (!candidate->isSolid() && (i == 1 || i == 2) &&
                (current->isLadder() || current->isRope() || isSolidUnderCurrent))
            value.push_back(candidate);
        else if (!candidate->isSolid() && i == 3)
            value.push_back(candidate);
        else if (!(candidate->isSolid()) && i == 0
                && current->isLadder())
            value.push_back(candidate);
    }
    return value;
}

Block* World::getBlock(int x, int y) {
    //Need to offset for the border
    return blocks[y * width + x];

}

Character* World::getPlayer() {
    return player;
}

void World::addBlock(int i, int j, int value) {
    Block* b = NULL;
    if (value >= 1 && value <= 6)
        b = new Block(value);
    else
        b = new Block(Block::EMPTY);

    b->setPosition(sf::Vector2f(i * Block::WIDTH, j * Block::HEIGHT));
    blocks.push_back(b);
}

