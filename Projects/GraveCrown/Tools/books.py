"""
The books, pages and letters of Grave Crown: what they say (read aloud by the lich of the court, Tools/narrate_books.py)
and where they are found (Tools/build_scene.py places them). The story is told sparingly, as in souls-likes: each text
is a piece of the betrayal of the dead king and a hint about the gods' servants
"""

# id: (title, where, text). 'where': "throne" lies by the throne, "drop:<enemy name>" is dropped by that enemy,
# "plains:x,z" lies on the plains
BOOKS = {
    "testament_of_the_king": (
        "Testament of the Dead King", "throne",
        "If you read this, my liege, you have risen. A hundred years ago the gods came down with their church and "
        "named you a tyrant. They bound the world in chains and hanged you before your own people. "
        "We, your court, kept your body and your crown. Rest by the fire, take up your blade, and go. "
        "The gods sleep on the backs of their titans now. Wake them."),
    "page_of_the_chain_codex": (
        "Page of the Chain Codex", "drop:Road Knight",
        "Every knight of the gods swears on the Chain: to hold, to bind, never to yield. "
        "Yet their guard is a vow and not a wall. Strike again and again, give them no breath, "
        "and the vow breaks. A knight who reels is a knight who can be ended."),
    "letter_of_a_cultist": (
        "Letter of a Masked Cultist", "drop:Gods' Cultist",
        "Brother, the Warden grows restless behind the fog. He swings his blade so hard the earth itself screams, "
        "and no shield, no parry, can turn those blows aside. When you see the red glint in his eye, run. "
        "Pray it is not you he is looking at."),
    "seal_of_the_warden": (
        "Seal of the Chain Warden", "drop:Chain Warden",
        "The Warden falls, and the first chain of the world snaps. Far to the east, under a burning sky, "
        "something enormous turns in its sleep. The god it carries has heard the chain break. "
        "He knows the king is coming."),
}

# The lich's voice (Kokoro: a British male voice, slow and grave)
VOICE = "bm_george"
SPEED = 0.82
