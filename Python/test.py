import pandas as pd
import torch
import os
from dataset import SentimentAnalysisDataset
from model import Model
from torch import cuda
from torch.utils.data import DataLoader
from transformers import RobertaTokenizer

device = 'cuda' if cuda.is_available() else 'cpu'

PATH = "0_5920/"
MODEL_PATH = path = os.path.join(PATH, "model.pth")
VALID_PATH = "data/valid.csv"
TEST_PATH = "data/test_no_label.csv"

VALID_BATCH_SIZE = 16
MAX_LEN = 256

valid_df = pd.read_csv(VALID_PATH)
valid_df["text"] = valid_df["text"].str.replace("\n", " ")

tokenizer = RobertaTokenizer.from_pretrained('roberta-base', truncation=True, do_lower_case=True)

valid_dataset = SentimentAnalysisDataset(valid_df, tokenizer, MAX_LEN)
valid_loader = DataLoader(valid_dataset, batch_size=VALID_BATCH_SIZE, shuffle=False, num_workers=0)

model = Model().to(device)
model.load_state_dict(torch.load(MODEL_PATH))
model.eval()
print(f"=== Load model: {MODEL_PATH} ===")

# Check Valid Acc
print("=== Start Checking Valid Acc ===")
print("valid size:", len(valid_df))
with torch.no_grad():
  valid_correct, valid_total = 0, 0
  for idx, sample in enumerate(valid_loader):
    input_ids = sample["input_ids"].to(device, dtype = torch.long)
    attention_mask = sample["attention_mask"].to(device, dtype = torch.long)
    token_type_ids = sample["token_type_ids"].to(device, dtype = torch.long)
    label = sample["label"]

    softmax_output = model(input_ids, attention_mask, token_type_ids)
    pred_label = torch.argmax(softmax_output, dim=1)

    pred_label = pred_label.data.cpu()

    valid_correct += (pred_label == label).sum().item()
    valid_total += label.size(0)
  print(f"valid_acc = {valid_correct} / {valid_total} = {(valid_correct/valid_total):.4f}")
print("=== Finish Checking Valid Acc ===")

# ===== Predition ===== #
test_df = pd.read_csv(TEST_PATH)
test_df["text"] = test_df["text"].str.replace("\n", " ")

pred_df = pd.DataFrame(columns=['id', 'label'])

print("=== Start Prediction ===")
print("test size:", len(test_df))
with torch.no_grad():
  for idx, sample in test_df.iterrows():
    id = sample["id"]
    text = sample["text"]

    inputs = tokenizer.encode_plus(
      text,
      None,
      add_special_tokens=True,
      max_length=MAX_LEN,
      padding='max_length',
      return_token_type_ids=True
    )

    input_ids = torch.unsqueeze(torch.tensor(inputs["input_ids"], dtype=torch.long), 0).to(device, dtype = torch.long)
    attention_mask = torch.unsqueeze(torch.tensor(inputs["attention_mask"], dtype=torch.long), 0).to(device, dtype = torch.long)
    token_type_ids = torch.unsqueeze(torch.tensor(inputs["token_type_ids"], dtype=torch.long), 0).to(device, dtype = torch.long)

    softmax_output = model(input_ids, attention_mask, token_type_ids)
    pred_label = torch.argmax(softmax_output, dim=1) + 1
    pred_label = pred_label.data.cpu().item()

    pred_df.loc[len(pred_df)] = [id, pred_label]

CSV_PATH = os.path.join(PATH, "pred.csv")
pred_df.to_csv(CSV_PATH, index=False)
print(f'=== Write prediction to {CSV_PATH} ===')
