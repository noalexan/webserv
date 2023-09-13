import pygame

def lire_et_jouer_audio(fichier_audio):
	pygame.mixer.init()
	pygame.mixer.music.load(fichier_audio)
	pygame.mixer.music.play()

if __name__ == "__main__":
	lire_et_jouer_audio("/Users/sihemayoub/Desktop/Ecole 42/webserv/public/assets/media/internet.mp3")

	while pygame.mixer.music.get_busy():
		continue
