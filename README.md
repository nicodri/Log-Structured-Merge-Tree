# Log Structured Merge Tree

Development project of an LSM-tree in C for the Harvard course CS 265 Big Data Systems.

This project aims to build a data structure providing low- cost indexing for a file experiencing a high rate of record inserts/deletes over an extended period: a Log-Structured Merge-tree (LSM-tree). Efficient memory based data structure are not possible with a large number of records, especially being able to access any element is a re- quirement of the system. The LSM-Tree faces the challenge as it operates on batch for the inserts/updates, cascading the changes from its first compononent C0 (memory based) to one or more larger components (disk-based). In this imple- mentation, each component uses a sorted array (except for the first component) as data structure and the data opera- tion READ, INSERT, UPDATE and DELETE are handled. The focus was on both having fast operations and a persis- tent implementation. The update rate is about 0.25M per second and the read rate about 22,5 K per second on flash storage.

The corresponding paper is available online: 
https://drive.google.com/open?id=0B8me976s9q7XOVRaU09oVnUyOGM

2016 - Nicolas Drizard
